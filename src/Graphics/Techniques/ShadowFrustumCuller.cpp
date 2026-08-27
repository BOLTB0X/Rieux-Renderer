#include "Pch.h"
#include "ShadowFrustumCuller.h"
// Tools
#include "DescriptorHeapAllocator.h"
// Data
#include "Frustum.h"
// Core
#include "RendererState.h"
// Utils
#include "GPUCommons.h"
#include "DebugHelper.h"

using namespace DirectX;
using namespace Microsoft::WRL;

ShadowFrustumCuller::ShadowFrustumCuller()
    : m_rootSignature(nullptr), m_computePSO(nullptr), m_maxMainCount(0), m_maxVaseCount(0), m_heapAllocator(nullptr),
    m_mainVisibleDescIndex{}, m_mainVisibleSRVIndex{}, m_vaseVisibleDescIndex{}, m_vaseVisibleSRVIndex{},
    m_mainCounterUAVIndex(0), m_mainCounterSRVIndex(0), m_vaseCounterUAVIndex(0), m_vaseCounterSRVIndex(0),
    m_cachedMainVisibleCounts{}, m_cachedVaseVisibleCounts{}, m_hasValidVisibleBuffers(false) {

    for (int i = 0; i < MAX_CASCADES; ++i) {
        m_cachedMainVisibleCounts[i] = 0;
        m_cachedVaseVisibleCounts[i] = 0;
        m_mainVisibleDescIndex[i] = UINT_MAX;
        m_vaseVisibleDescIndex[i] = UINT_MAX;
    }
} // ShadowFrustumCuller

ShadowFrustumCuller::~ShadowFrustumCuller() {
    m_rootSignature = nullptr;
    m_computePSO = nullptr;
    m_heapAllocator = nullptr;
} // ~ShadowFrustumCuller

bool ShadowFrustumCuller::Init(const InitParams& params) {
    if (!params.device || params.maxMainCount == 0 || !params.rootSig || !params.pso || !params.heapAllocator) {
        DebugHelper::DebugPrint("ShadowFrustumCuller Init 실패: 잘못된 인자");
        return false;
    }

    m_maxMainCount = params.maxMainCount;
    m_maxVaseCount = params.maxVaseCount;
    m_rootSignature = params.rootSig;
    m_computePSO = params.pso;
    m_heapAllocator = params.heapAllocator;

    for (size_t i = 0; i < MAX_CASCADES; ++i) {
        m_frustums[i] = std::make_unique<Frustum>();
        m_frustums[i]->Init(RendererState::ScreenDepth, true);
    }
    BuildBuffers(params.device);
    return true;
} // Init

void ShadowFrustumCuller::Frame(const std::array<DirectX::XMMATRIX, MAX_CASCADES>& cascadeView,
    const std::array<DirectX::XMMATRIX, MAX_CASCADES>& cascadeProj) {

    ShadowCullingCB cbData = {};
    cbData.mainInstances = m_maxMainCount;
    cbData.vaseInstances = m_maxVaseCount;

    for (size_t i = 0; i < MAX_CASCADES; ++i) {
        m_frustums[i]->Frame(cascadeView[i], cascadeProj[i]);
        memcpy(cbData.cascadePlanes[i], m_frustums[i]->GetPlanes(), sizeof(DirectX::XMFLOAT4) * 6);
    }

    void* mappedData = nullptr;
    if (SUCCEEDED(m_shadowConstantBuffer->Map(0, nullptr, &mappedData))) {
        memcpy(mappedData, &cbData, sizeof(ShadowCullingCB));
        m_shadowConstantBuffer->Unmap(0, nullptr);
    }
} // Frame

void ShadowFrustumCuller::Dispatch(const DispatchParams& param) {
    auto cmdList = param.cmdList;
    if (!cmdList || !m_heapAllocator || (m_maxMainCount + m_maxVaseCount) == 0) return;

    // 카운터 초기화 (UAV 쓰기 전)
    D3D12_RESOURCE_BARRIER preCopy[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
    };
    cmdList->ResourceBarrier(2, preCopy);

    // 4개의 UINT(16바이트) 초기화 (Main, Vase 둘 다 리셋 버퍼 활용)
    cmdList->CopyBufferRegion(m_mainCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT) * 4);
    cmdList->CopyBufferRegion(m_vaseCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT) * 4);

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    D3D12_RESOURCE_STATES initialResourceState = m_hasValidVisibleBuffers
        ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        : D3D12_RESOURCE_STATE_COMMON;

    for (int i = 0; i < MAX_CASCADES; ++i) {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainVisibleCommandsCascade[i].Get(), initialResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseVisibleCommandsCascade[i].Get(), initialResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }
    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // 파이프라인 및 Descriptor 바인딩
    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);

    cmdList->SetComputeRootConstantBufferView(RendererState::ShadowCullingDataIndex, m_shadowConstantBuffer->GetGPUVirtualAddress());

    if (param.instanceDescIndex != UINT_MAX)
        cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMasterInstanceIndex, m_heapAllocator->GetGPUHandle(param.instanceDescIndex));
    if (param.masterIndirectDescriptorIndex != UINT_MAX)
        cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMasterCommandsIndex, m_heapAllocator->GetGPUHandle(param.masterIndirectDescriptorIndex));

    // Main Command Buffers
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMainVisibleCommandsCascade0Index, m_heapAllocator->GetGPUHandle(m_mainVisibleDescIndex[0]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMainVisibleCommandsCascade1Index, m_heapAllocator->GetGPUHandle(m_mainVisibleDescIndex[1]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMainVisibleCommandsCascade2Index, m_heapAllocator->GetGPUHandle(m_mainVisibleDescIndex[2]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMainVisibleCommandsCascade3Index, m_heapAllocator->GetGPUHandle(m_mainVisibleDescIndex[3]));

    // Vase Command Buffers
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingVaseVisibleCommandsCascade0Index, m_heapAllocator->GetGPUHandle(m_vaseVisibleDescIndex[0]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingVaseVisibleCommandsCascade1Index, m_heapAllocator->GetGPUHandle(m_vaseVisibleDescIndex[1]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingVaseVisibleCommandsCascade2Index, m_heapAllocator->GetGPUHandle(m_vaseVisibleDescIndex[2]));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingVaseVisibleCommandsCascade3Index, m_heapAllocator->GetGPUHandle(m_vaseVisibleDescIndex[3]));

    // Counters (Main & Vase)
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingMainDrawCountsIndex, m_heapAllocator->GetGPUHandle(m_mainCounterUAVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::ShadowCullingVaseDrawCountsIndex, m_heapAllocator->GetGPUHandle(m_vaseCounterUAVIndex));

    // 단일 Dispatch 호출
    UINT totalInstances = m_maxMainCount + m_maxVaseCount;
    cmdList->Dispatch((totalInstances + 63) / 64, 1, 1);

    // 간접 그리기를 위한 상태 복구 (UAV -> SRV)
    for (auto& barrier : barriers) {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    m_hasValidVisibleBuffers = true;
} // Dispatch

void ShadowFrustumCuller::PrepareForIndirectDraw(ID3D12GraphicsCommandList* cmdList) {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));

    for (int i = 0; i < MAX_CASCADES; ++i) {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainVisibleCommandsCascade[i].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseVisibleCommandsCascade[i].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));
    }
    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
} // PrepareForIndirectDraw

void ShadowFrustumCuller::RestoreAfterIndirectDraw(ID3D12GraphicsCommandList* cmdList) {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

    for (int i = 0; i < MAX_CASCADES; ++i) {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_mainVisibleCommandsCascade[i].Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(m_vaseVisibleCommandsCascade[i].Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    }
    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
} // RestoreAfterIndirectDraw

void ShadowFrustumCuller::ReadbackToCPU(ID3D12GraphicsCommandList* cmdList) {
    D3D12_RESOURCE_BARRIER toCopySrc[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    cmdList->ResourceBarrier(2, toCopySrc);

    cmdList->CopyResource(m_mainReadbackBuffer.Get(), m_mainCounterBuffer.Get());
    cmdList->CopyResource(m_vaseReadbackBuffer.Get(), m_vaseCounterBuffer.Get());

    D3D12_RESOURCE_BARRIER back[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_vaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    cmdList->ResourceBarrier(2, back);
} // ReadbackToCPU

void ShadowFrustumCuller::OnGUI() {
    UINT* pMainCounts = nullptr;
    if (SUCCEEDED(m_mainReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMainCounts)))) {
        for (int i = 0; i < MAX_CASCADES; ++i) m_cachedMainVisibleCounts[i] = pMainCounts[i];
        m_mainReadbackBuffer->Unmap(0, nullptr);
    }

    UINT* pVaseCounts = nullptr;
    if (SUCCEEDED(m_vaseReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVaseCounts)))) {
        for (int i = 0; i < MAX_CASCADES; ++i) m_cachedVaseVisibleCounts[i] = pVaseCounts[i];
        m_vaseReadbackBuffer->Unmap(0, nullptr);
    }

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[ Shadow Frustum Culler ]");
    for (int i = 0; i < MAX_CASCADES; ++i) {
        ImGui::Text("Cascade %d | Main: %u | Vase: %u", i, m_cachedMainVisibleCounts[i], m_cachedVaseVisibleCounts[i]);
    }
    ImGui::Separator();
} // OnGUI

void ShadowFrustumCuller::BuildBuffers(ID3D12Device* device) {
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);

    const UINT cbSize = (sizeof(ShadowCullingCB) + 255) & ~255;
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_shadowConstantBuffer));

    CD3DX12_RESOURCE_DESC resetDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * 4);
    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resetDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_counterResetBuffer));

    UINT* pResetData = nullptr;
    if (SUCCEEDED(m_counterResetBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pResetData)))) {
        memset(pResetData, 0, sizeof(UINT) * 4);
        m_counterResetBuffer->Unmap(0, nullptr);
    }

    // Readback 버퍼 생성 (Main, Vase)
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &resetDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_mainReadbackBuffer));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &resetDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vaseReadbackBuffer));

    // Counter 생성
    auto CreateCounterBuffer = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, UINT& uavIdx, UINT& srvIdx) {
        CD3DX12_RESOURCE_DESC counterDesc = CD3DX12_RESOURCE_DESC::Buffer(
            sizeof(UINT) * 4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &counterDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));
            //D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&buffer));

        uavIdx = m_heapAllocator->Allocate();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = 4;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        device->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, m_heapAllocator->GetCPUHandle(uavIdx));

        srvIdx = m_heapAllocator->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = 4;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(buffer.Get(), &srvDesc, m_heapAllocator->GetCPUHandle(srvIdx));
        };

    CreateCounterBuffer(m_mainCounterBuffer, m_mainCounterUAVIndex, m_mainCounterSRVIndex);
    CreateCounterBuffer(m_vaseCounterBuffer, m_vaseCounterUAVIndex, m_vaseCounterSRVIndex);

    // 4개의 캐스케이드용 개별 Visible Command 버퍼 생성
    for (int i = 0; i < MAX_CASCADES; ++i) {
        BuildGroupResources(device, m_maxMainCount, i, m_mainVisibleCommandsCascade[i], m_mainVisibleDescIndex[i], m_mainVisibleSRVIndex[i]);
        BuildGroupResources(device, m_maxVaseCount, i, m_vaseVisibleCommandsCascade[i], m_vaseVisibleDescIndex[i], m_vaseVisibleSRVIndex[i]);
    }
} // BuildBuffers

void ShadowFrustumCuller::BuildGroupResources(ID3D12Device* device, UINT maxCount, UINT cascadeIndex,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outBuffer,
    UINT& outDescIndex, UINT& outSRVIndex) {
    if (maxCount == 0) return;

    UINT commandStructSize = sizeof(GPUCommons::IndirectCommand);
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC visibleDesc = CD3DX12_RESOURCE_DESC::Buffer(
        commandStructSize * maxCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &visibleDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outBuffer));

    outDescIndex = m_heapAllocator->Allocate();
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescView = {};
    uavDescView.Format = DXGI_FORMAT_UNKNOWN;
    uavDescView.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDescView.Buffer.NumElements = maxCount;
    uavDescView.Buffer.StructureByteStride = commandStructSize;
    device->CreateUnorderedAccessView(outBuffer.Get(), nullptr, &uavDescView,
        m_heapAllocator->GetCPUHandle(outDescIndex));

    outSRVIndex = m_heapAllocator->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescView = {};
    srvDescView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescView.Format = DXGI_FORMAT_UNKNOWN;
    srvDescView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescView.Buffer.NumElements = maxCount;
    srvDescView.Buffer.StructureByteStride = commandStructSize;
    device->CreateShaderResourceView(outBuffer.Get(), &srvDescView,
        m_heapAllocator->GetCPUHandle(outSRVIndex));
} // BuildGroupResources

ID3D12Resource* ShadowFrustumCuller::GetMainVisibleCommandsBuffer(UINT cascadeIndex) const { return m_mainVisibleCommandsCascade[cascadeIndex].Get(); }
ID3D12Resource* ShadowFrustumCuller::GetVaseVisibleCommandsBuffer(UINT cascadeIndex) const { return m_vaseVisibleCommandsCascade[cascadeIndex].Get(); }
ID3D12Resource* ShadowFrustumCuller::GetMainCounterBuffer() const { return m_mainCounterBuffer.Get(); }
ID3D12Resource* ShadowFrustumCuller::GetVaseCounterBuffer() const { return m_vaseCounterBuffer.Get(); }
UINT            ShadowFrustumCuller::GetCounterBufferOffset(UINT cascadeIndex) const { return cascadeIndex * sizeof(UINT); }
UINT            ShadowFrustumCuller::GetMainCounterSRVIndex() const { return m_mainCounterSRVIndex; }
UINT            ShadowFrustumCuller::GetVaseCounterSRVIndex() const { return m_vaseCounterSRVIndex; }