#include "Pch.h"
#include "FrustumCuller.h"
// Data
#include "Frustum.h"
// Components
#include "DescriptorHeapAllocator.h"
// Core
#include "RendererState.h"
// Commons
#include "Sponza.h"
// Utils
#include "GPUCommons.h"
#include "DebugHelper.h"

using namespace DirectX;
using namespace Microsoft::WRL;

FrustumCuller::FrustumCuller()
    : m_rootSignature(nullptr), m_computePSO(nullptr),
    m_mainVisibleDescIndex(UINT_MAX), m_mainCounterDescIndex(UINT_MAX),
    m_vaseVisibleDescIndex(UINT_MAX), m_vaseCounterDescIndex(UINT_MAX),
    m_maxMainCount(0), m_maxVaseCount(0), m_cachedMainVisibleCount(0), m_cachedVaseVisibleCount(0),
    m_heapAllocator(nullptr) {
    m_frustum = std::make_unique<Frustum>();
} // FrustumCuller

FrustumCuller::~FrustumCuller() {
} // ~FrustumCuller

bool FrustumCuller::Init(const InitParams& params) {
    if (!params.device || params.maxMainCount == 0 || params.maxVaseCount == 0
        || !params.rootSig || !params.pso || !params.heapAllocator) {
        DebugHelper::DebugPrint("FrustumCuller Init 실패: 잘못된 인자");
        return false;
    }

    m_maxMainCount = params.maxMainCount;
	m_maxVaseCount = params.maxVaseCount;
    m_rootSignature = params.rootSig;
    m_computePSO = params.pso;
    m_heapAllocator = params.heapAllocator;

    m_frustum->Init(RendererState::ScreenDepth);
    BuildBuffers(params.device);
    return true;
} // Init

void FrustumCuller::Frame(XMMATRIX viewMatrix, XMMATRIX projectionMatrix) {
    m_frustum->Frame(viewMatrix, projectionMatrix);
} // Frame

void FrustumCuller::Dispatch(const DispatchParams& param) {
    auto cmdList = param.cmdList;
    if (!cmdList || !m_heapAllocator) {
        return;
    }

    UINT totalInstances = m_maxMainCount + m_maxVaseCount;
    if (totalInstances == 0) {
        return;
    }

    GPUCommons::FrustumCullingCB cbData = {};
    const auto* planes = m_frustum->GetPlanes();
    for (int i = 0; i < 6; ++i) {
        cbData.planes[i] = planes[i];
    }
    cbData.totalInstances = totalInstances;

    void* mappedData = nullptr;
    if (SUCCEEDED(m_frustumConstantBuffer->Map(0, nullptr, &mappedData))) {
        memcpy(mappedData, &cbData, sizeof(GPUCommons::FrustumCullingCB));
        m_frustumConstantBuffer->Unmap(0, nullptr);
    }

    D3D12_RESOURCE_BARRIER toCopyDest[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainVisibleCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseVisibleCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(4, toCopyDest);

    cmdList->CopyBufferRegion(m_mainCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));
    cmdList->CopyBufferRegion(m_vaseCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));

    D3D12_RESOURCE_BARRIER toUAV[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(2, toUAV);

    // 파이프라인 및 바인딩
    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);
    cmdList->SetComputeRootConstantBufferView(RendererState::CullingFrustumPlanesIndex, m_frustumConstantBuffer->GetGPUVirtualAddress());

    if (param.instanceDescIndex != UINT_MAX) {
        cmdList->SetComputeRootDescriptorTable(RendererState::CullingMasterInstanceIndex, m_heapAllocator->GetGPUHandle(param.instanceDescIndex));
    }

    if (param.masterIndirectDescriptorIndex != UINT_MAX) {
        cmdList->SetComputeRootDescriptorTable(RendererState::CullingMasterCommandsIndex, m_heapAllocator->GetGPUHandle(param.masterIndirectDescriptorIndex));
    }

    cmdList->SetComputeRootDescriptorTable(RendererState::CullingVisibleMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_mainVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::CullingVisibleVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_vaseVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::CullingMainCountIndex, m_heapAllocator->GetGPUHandle(m_mainCounterDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::CullingVaseCountIndex, m_heapAllocator->GetGPUHandle(m_vaseCounterDescIndex));

    // 디스패치
    cmdList->Dispatch((totalInstances + 63) / 64, 1, 1);

    // 결과 버퍼 복구
    D3D12_RESOURCE_BARRIER restore[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainVisibleCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseVisibleCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(4, restore);
} // Dispatch

void FrustumCuller::ReadbackToCPU(ID3D12GraphicsCommandList* cmdList) {
    D3D12_RESOURCE_BARRIER toCopySrc[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    cmdList->ResourceBarrier(2, toCopySrc);

    cmdList->CopyResource(m_mainReadbackBuffer.Get(), m_mainCounterBuffer.Get());
    cmdList->CopyResource(m_vaseReadbackBuffer.Get(), m_vaseCounterBuffer.Get());

    // 복사 후 다시 INDIRECT_ARGUMENT 복구
    D3D12_RESOURCE_BARRIER back[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(2, back);
} // ReadbackToCPU

void FrustumCuller::OnGUI() {
    UINT* pMain = nullptr;
    if (SUCCEEDED(m_mainReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMain)))) {
        m_cachedMainVisibleCount = *pMain;
        m_mainReadbackBuffer->Unmap(0, nullptr);
    }
    UINT* pVase = nullptr;
    if (SUCCEEDED(m_vaseReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVase)))) {
        m_cachedVaseVisibleCount = *pVase;
        m_vaseReadbackBuffer->Unmap(0, nullptr);
    }

    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "[ FrustumCuller Options ]");
    ImGui::Text("Main Visible: %u", m_cachedMainVisibleCount);
    ImGui::Text("Vase Visible: %u", m_cachedVaseVisibleCount);
    ImGui::Separator();
} // OnGUI

Frustum*        FrustumCuller::GetFrustum() const { return m_frustum.get(); }
ID3D12Resource* FrustumCuller::GetMainVisibleCommandsBuffer() const { return m_mainVisibleCommandsBuffer.Get(); }
ID3D12Resource* FrustumCuller::GetVaseVisibleCommandsBuffer() const { return m_vaseVisibleCommandsBuffer.Get(); }
ID3D12Resource* FrustumCuller::GetMainCounterBuffer() const { return m_mainCounterBuffer.Get(); }
ID3D12Resource* FrustumCuller::GetVaseCounterBuffer() const { return m_vaseCounterBuffer.Get(); }

void FrustumCuller::BuildGroupResources(ID3D12Device* device, UINT maxCount,
    ComPtr<ID3D12Resource>& outVisibleBuf, UINT& outVisibleDescIdx,
    ComPtr<ID3D12Resource>& outCounterBuf, UINT& outCounterDescIdx) {

    UINT commandStructSize = sizeof(GPUCommons::IndirectCommand);
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC visibleDesc = CD3DX12_RESOURCE_DESC::Buffer(
        commandStructSize * maxCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &visibleDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outVisibleBuf));

    CD3DX12_RESOURCE_DESC counterDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &counterDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outCounterBuf));

    // Visible Commands UAV
    // Structured Buffer 형태
    outVisibleDescIdx = m_heapAllocator->Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescView = {};
    uavDescView.Format = DXGI_FORMAT_UNKNOWN;
    uavDescView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDescView.Buffer.FirstElement = 0;
    uavDescView.Buffer.NumElements = maxCount;
    uavDescView.Buffer.StructureByteStride = commandStructSize;
    uavDescView.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    device->CreateUnorderedAccessView(outVisibleBuf.Get(), nullptr, &uavDescView,
        m_heapAllocator->GetCPUHandle(outVisibleDescIdx));

    // Counter UAV
    // RWByteAddressBuffer / RAW 형태
    outCounterDescIdx = m_heapAllocator->Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = {};
    counterUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    counterUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    counterUavDesc.Buffer.FirstElement = 0;
    counterUavDesc.Buffer.NumElements = 1; // 32비트(4바이트) 요소의 개수
    counterUavDesc.Buffer.StructureByteStride = 0;
    counterUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device->CreateUnorderedAccessView(outCounterBuf.Get(), nullptr, &counterUavDesc,
        m_heapAllocator->GetCPUHandle(outCounterDescIdx));
} // BuildGroupResources

void FrustumCuller::BuildBuffers(ID3D12Device* device) {
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    // Frustum 상수 버퍼
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(256);
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_frustumConstantBuffer));

    // Counter Reset 버퍼 (main/vase 공용, 0 고정)
    CD3DX12_RESOURCE_DESC resetDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resetDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_counterResetBuffer));

    UINT* pResetData = nullptr;
    m_counterResetBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pResetData));
    if (pResetData) {
        *pResetData = 0;
        m_counterResetBuffer->Unmap(0, nullptr);
    }

    // main/vase 각각의 visible+counter 버퍼/UAV 생성
    BuildGroupResources(device, m_maxMainCount, m_mainVisibleCommandsBuffer, m_mainVisibleDescIndex, m_mainCounterBuffer, m_mainCounterDescIndex);
    BuildGroupResources(device, m_maxVaseCount, m_vaseVisibleCommandsBuffer, m_vaseVisibleDescIndex, m_vaseCounterBuffer, m_vaseCounterDescIndex);

    // Readback (main/vase 각각)
    CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_mainReadbackBuffer));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vaseReadbackBuffer));
} // BuildBuffers