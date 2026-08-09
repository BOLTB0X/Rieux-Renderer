#include "Pch.h"
#include "OcclusionCuller.h"
#include "DescriptorHeapAllocator.h"
#include "RendererState.h"
// Utils
#include "DebugHelper.h"
#include "GPUCommons.h"

using namespace Microsoft::WRL;
using namespace DirectX;

OcclusionCuller::OcclusionCuller()
    : m_rootSignature(nullptr), m_computePSO(nullptr),
    m_finalMainVisibleDescIndex(UINT_MAX), m_finalMainCounterDescIndex(UINT_MAX),
    m_finalVaseVisibleDescIndex(UINT_MAX), m_finalVaseCounterDescIndex(UINT_MAX),
    m_maxMainCount(0), m_maxVaseCount(0), m_cachedMainVisibleCount(0), m_cachedVaseVisibleCount(0),
    m_heapAllocator(nullptr) {
} // OcclusionCuller

OcclusionCuller::~OcclusionCuller() {
    m_rootSignature = nullptr;
    m_computePSO = nullptr;
    m_heapAllocator = nullptr;
} // ~OcclusionCuller

bool OcclusionCuller::Init(const InitParams& params) {
    if (!params.device || params.maxMainCount == 0 || params.maxVaseCount == 0
        || !params.rootSig || !params.pso || !params.heapAllocator) {
        DebugHelper::DebugPrint("OcclusionCuller Init 실패: 잘못된 인자");
        return false;
    }

    m_maxMainCount = params.maxMainCount;
    m_maxVaseCount = params.maxVaseCount;
    m_rootSignature = params.rootSig;
    m_computePSO = params.pso;
    m_heapAllocator = params.heapAllocator;

    BuildBuffers(params.device);
    return true;
} // Init

void OcclusionCuller::Frame(const FrameParams& params) {
    GPUCommons::OcclusionCullingCB cbData = {};
    cbData.viewProj = XMMatrixTranspose(params.viewMatrix * params.projectionMatrix);
    cbData.screenWidth = params.screenWidth;
    cbData.screenHeight = params.screenHeight;

    void* mappedData = nullptr;
    if (SUCCEEDED(m_occlusionConstantBuffer->Map(0, nullptr, &mappedData))) {
        memcpy(mappedData, &cbData, sizeof(GPUCommons::OcclusionCullingCB));
        m_occlusionConstantBuffer->Unmap(0, nullptr);
    }
} // Frame

void OcclusionCuller::Dispatch(const DispatchParams& param) {
    auto cmdList = param.cmdList;
    if (!cmdList || !m_heapAllocator) {
        return;
    }

    UINT totalInstances = m_maxMainCount + m_maxVaseCount;
    if (totalInstances == 0) {
        return;
    }

    // 1. 상태 전환: 내 결과물(UAV) 복사 준비
    D3D12_RESOURCE_BARRIER toCopyDest[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(4, toCopyDest);

    // 2. 카운터 초기화
    cmdList->CopyBufferRegion(m_finalMainCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));
    cmdList->CopyBufferRegion(m_finalVaseCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));

    // 3. 상태 전환: 카운터를 UAV로 변경
    D3D12_RESOURCE_BARRIER toUAV[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(2, toUAV);

    // 4. 파이프라인 및 바인딩 (RendererState 인덱스 활용)
    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);

    // [상수 버퍼] b0
    cmdList->SetComputeRootConstantBufferView(RendererState::OcclusionConstantsIndex, m_occlusionConstantBuffer->GetGPUVirtualAddress());

    // [SRV 입력 바인딩] t0 ~ t5
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionHiZTextureIndex, m_heapAllocator->GetGPUHandle(param.hizTextureDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCommandsIndex, m_heapAllocator->GetGPUHandle(param.frustumMainCommandsDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCommandsIndex, m_heapAllocator->GetGPUHandle(param.frustumVaseCommandsDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCountIndex, m_heapAllocator->GetGPUHandle(param.frustumMainCountDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCountIndex, m_heapAllocator->GetGPUHandle(param.frustumVaseCountDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionMeshInstanceDataIndex, m_heapAllocator->GetGPUHandle(param.meshInstanceDataDescIndex));

    // [UAV 출력 바인딩] u0 ~ u3
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalMainVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalVaseVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCountIndex, m_heapAllocator->GetGPUHandle(m_finalMainCounterDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCountIndex, m_heapAllocator->GetGPUHandle(m_finalVaseCounterDescIndex));

    // 5. 디스패치
    cmdList->Dispatch((totalInstances + 63) / 64, 1, 1);

    // 6. 상태 복구 (ExecuteIndirect용)
    D3D12_RESOURCE_BARRIER restore[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(4, restore);
}

void OcclusionCuller::ReadbackToCPU(ID3D12GraphicsCommandList* cmdList) {
    D3D12_RESOURCE_BARRIER toCopySrc[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    cmdList->ResourceBarrier(2, toCopySrc);

    cmdList->CopyResource(m_mainReadbackBuffer.Get(), m_finalMainCounterBuffer.Get());
    cmdList->CopyResource(m_vaseReadbackBuffer.Get(), m_finalVaseCounterBuffer.Get());

    D3D12_RESOURCE_BARRIER back[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(2, back);
} // ReadbackToCPU

ID3D12Resource* OcclusionCuller::GetFinalMainCommandsBuffer() const { return m_finalMainCommandsBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalVaseCommandsBuffer() const { return m_finalVaseCommandsBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalMainCounterBuffer() const { return m_finalMainCounterBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalVaseCounterBuffer() const { return m_finalVaseCounterBuffer.Get(); }

void OcclusionCuller::BuildGroupResources(ID3D12Device* device, UINT maxCount,
    ComPtr<ID3D12Resource>& outVisibleBuf, UINT& outVisibleDescIdx,
    ComPtr<ID3D12Resource>& outCounterBuf, UINT& outCounterDescIdx) {

    // FrustumCuller의 구현과 완전히 동일하게 UAV를 생성
    UINT commandStructSize = sizeof(GPUCommons::IndirectCommand); // 선언된 구조체에 맞춤
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC visibleDesc = CD3DX12_RESOURCE_DESC::Buffer(
        commandStructSize * maxCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &visibleDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outVisibleBuf));

    CD3DX12_RESOURCE_DESC counterDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &counterDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outCounterBuf));

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

    outCounterDescIdx = m_heapAllocator->Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = {};
    counterUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    counterUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    counterUavDesc.Buffer.FirstElement = 0;
    counterUavDesc.Buffer.NumElements = 1;
    counterUavDesc.Buffer.StructureByteStride = 0;
    counterUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device->CreateUnorderedAccessView(outCounterBuf.Get(), nullptr, &counterUavDesc,
        m_heapAllocator->GetCPUHandle(outCounterDescIdx));
} // BuildGroupResources

void OcclusionCuller::BuildBuffers(ID3D12Device* device) {
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    // Occlusion 상수 버퍼 (ViewProj, Screen Size 등)
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(256);
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_occlusionConstantBuffer));

    // Counter Reset 버퍼
    CD3DX12_RESOURCE_DESC resetDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resetDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_counterResetBuffer));

    UINT* pResetData = nullptr;
    m_counterResetBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pResetData));
    if (pResetData) {
        *pResetData = 0;
        m_counterResetBuffer->Unmap(0, nullptr);
    }

    BuildGroupResources(device, m_maxMainCount, m_finalMainCommandsBuffer, m_finalMainVisibleDescIndex, m_finalMainCounterBuffer, m_finalMainCounterDescIndex);
    BuildGroupResources(device, m_maxVaseCount, m_finalVaseCommandsBuffer, m_finalVaseVisibleDescIndex, m_finalVaseCounterBuffer, m_finalVaseCounterDescIndex);

    CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_mainReadbackBuffer));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vaseReadbackBuffer));
} // BuildBuffers

void OcclusionCuller::OnGUI() {
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

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ OcclusionCuller Options ]");
    ImGui::Text("Final Main Visible: %u", m_cachedMainVisibleCount);
    ImGui::Text("Final Vase Visible: %u", m_cachedVaseVisibleCount);
    ImGui::Separator();
} // OnGUI