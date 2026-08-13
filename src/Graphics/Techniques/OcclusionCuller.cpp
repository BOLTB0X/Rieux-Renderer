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
    m_phase1CulledMainUAVIndex(UINT_MAX), m_phase1CulledMainCounterUAVIndex(UINT_MAX),
    m_phase1CulledMainSRVIndex(UINT_MAX), m_phase1CulledMainCounterSRVIndex(UINT_MAX),
    m_phase1CulledVaseUAVIndex(UINT_MAX), m_phase1CulledVaseCounterUAVIndex(UINT_MAX),
    m_phase1CulledVaseSRVIndex(UINT_MAX), m_phase1CulledVaseCounterSRVIndex(UINT_MAX),
    m_dummyUAVDescIndex(0),
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
    cbData.mainCapacity = m_maxMainCount;
    cbData.vaseCapacity = m_maxVaseCount;

    void* mappedData = nullptr;
    if (SUCCEEDED(m_occlusionConstantBuffer->Map(0, nullptr, &mappedData))) {
        memcpy(mappedData, &cbData, sizeof(GPUCommons::OcclusionCullingCB));
        m_occlusionConstantBuffer->Unmap(0, nullptr);
    }
} // Frame

void OcclusionCuller::DispatchPhase1(const DispatchPhase1Params& param) {
    auto cmdList = param.cmdList;
    UINT totalInstances = m_maxMainCount + m_maxVaseCount;
    if (!cmdList || !m_heapAllocator || totalInstances == 0) return;

    // 카운터 초기화를 위해 Copy Dest로 상태 변경
    D3D12_RESOURCE_BARRIER toCopyDest[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
    };
    cmdList->ResourceBarrier(4, toCopyDest);

    // 카운터를 0으로 싹 다 초기화
    cmdList->CopyBufferRegion(m_finalMainCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));
    cmdList->CopyBufferRegion(m_finalVaseCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));
    cmdList->CopyBufferRegion(m_phase1CulledMainCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));
    cmdList->CopyBufferRegion(m_phase1CulledVaseCounterBuffer.Get(), 0, m_counterResetBuffer.Get(), 0, sizeof(UINT));

    // Command 버퍼와 Counter 버퍼들을 모두 UAV로 전환
    D3D12_RESOURCE_BARRIER toUAV[8] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(8, toUAV);

    // 파이프라인 세팅
    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);
    cmdList->SetComputeRootConstantBufferView(
        RendererState::OcclusionConstantsIndex,
        m_occlusionConstantBuffer->GetGPUVirtualAddress());

    // Phase 1 바인딩 시작
    // b1: Phase Index (0: Phase1)
    uint32_t phaseInfo[2] = { 0, param.hasPreviousHiz ? 1u : 0u };
    cmdList->SetComputeRoot32BitConstants(RendererState::OcclusionPhaseIndex, 2, phaseInfo, 0);

    // t0: 이전 프레임 Hi-Z 텍스처
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionHiZTextureIndex, m_heapAllocator->GetGPUHandle(param.previousHizTextureDescIndex));

    // t1~t4: Frustum 통과 결과 (입력)
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCommandsIndex, m_heapAllocator->GetGPUHandle(param.frustumMainCommandsDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCommandsIndex, m_heapAllocator->GetGPUHandle(param.frustumVaseCommandsDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCountIndex, m_heapAllocator->GetGPUHandle(param.frustumMainCountDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCountIndex, m_heapAllocator->GetGPUHandle(param.frustumVaseCountDescIndex));

    // t5: 인스턴스 데이터
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionMeshInstanceDataIndex, m_heapAllocator->GetGPUHandle(param.meshInstanceDataDescIndex));

    // u0~u3: 최종 통과 버퍼 (Final) - 수정된 부분
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalMainVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalVaseVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCountIndex, m_heapAllocator->GetGPUHandle(m_finalMainCounterDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCountIndex, m_heapAllocator->GetGPUHandle(m_finalVaseCounterDescIndex));

    // u4~u7: Phase1 실패작 버퍼 (Culled)
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledMainUAVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledVaseUAVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledMainCountIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledMainCounterUAVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledVaseCountIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledVaseCounterUAVIndex));

    // 디스패치
    cmdList->Dispatch((totalInstances + 63) / 64, 1, 1);

    // Phase 1 직후 Barrier 전환
    D3D12_RESOURCE_BARRIER toPhase1End[8] = {
        // 1~4: Final 버퍼들은 Depth/Sponza에서 Indirect Argument로 사용하기 위해 전환
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),

        // 5~8: Phase 1 Culled 버퍼들 Command & Counter은 Phase 2에서 SRV 위해 NON_PIXEL_SHADER_RESOURCE로 전환
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_phase1CulledVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    cmdList->ResourceBarrier(8, toPhase1End);
} // DispatchPhase1

void OcclusionCuller::DispatchPhase2(const DispatchPhase2Params& param) {
    auto cmdList = param.cmdList;
    if (!cmdList || !m_heapAllocator) return;

    // Phase 1에서 그렸던 Final 버퍼를 다시 Append하기 위해 UAV로 전환
    D3D12_RESOURCE_BARRIER toUAV[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    cmdList->ResourceBarrier(4, toUAV);

    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);
    cmdList->SetComputeRootConstantBufferView(
        RendererState::OcclusionConstantsIndex,
        m_occlusionConstantBuffer->GetGPUVirtualAddress());

    // Phase 2 바인딩 시작
    // b1: Phase Index (1: Phase2)
    uint32_t phaseInfo[2] = { 1, 1 };
    cmdList->SetComputeRoot32BitConstants(RendererState::OcclusionPhaseIndex, 2, phaseInfo, 0);

    // t0: 이번 프레임(방금 만든) Hi-Z 텍스처
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionHiZTextureIndex, m_heapAllocator->GetGPUHandle(param.currentHizTextureDescIndex));

    // t1~t4: Frustum 데이터 대신 Phase 1에서 실패한 Culled
    // SRV를 입력으로 사용
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledMainSRVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledVaseSRVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumMainCountIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledMainCounterSRVIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFrustumVaseCountIndex, m_heapAllocator->GetGPUHandle(m_phase1CulledVaseCounterSRVIndex));

    // t5: 인스턴스 데이터
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionMeshInstanceDataIndex, m_heapAllocator->GetGPUHandle(param.meshInstanceDataDescIndex));

    // u0~u3: 최종 통과 버퍼
    // 여기에 계속 Append 됨
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalMainVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_finalVaseVisibleDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalMainCountIndex, m_heapAllocator->GetGPUHandle(m_finalMainCounterDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionFinalVaseCountIndex, m_heapAllocator->GetGPUHandle(m_finalVaseCounterDescIndex));

    // u4~u7: Phase 2에서는 실패작
    // Culled을 기록하지 않으므로 사용 안 함.
    // 단, D3D12 유효성 검사 경고를 피하기 위해 이미 바인딩된 Final UAV를 더미로 채워줌
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledMainCommandsIndex, m_heapAllocator->GetGPUHandle(m_dummyUAVDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledVaseCommandsIndex, m_heapAllocator->GetGPUHandle(m_dummyUAVDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledMainCountIndex, m_heapAllocator->GetGPUHandle(m_dummyUAVDescIndex));
    cmdList->SetComputeRootDescriptorTable(RendererState::OcclusionCulledVaseCountIndex, m_heapAllocator->GetGPUHandle(m_dummyUAVDescIndex));

    // 디스패치
    UINT totalInstances = m_maxMainCount + m_maxVaseCount;
    cmdList->Dispatch((totalInstances + 63) / 64, 1, 1);

    // 최종 렌더링(메인 씬 Draw)을 위해 모두 다시 Indirect Argument로 전환
    D3D12_RESOURCE_BARRIER toFinal[4] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCommandsBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalMainCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        CD3DX12_RESOURCE_BARRIER::Transition(m_finalVaseCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    cmdList->ResourceBarrier(4, toFinal);
} // DispatchPhase2

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

    UINT* mappedMainCount = nullptr;
    UINT* mappedVaseCount = nullptr;

    if (SUCCEEDED(m_mainReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedMainCount)))) {
        UINT finalMainCount = *mappedMainCount;
        m_mainReadbackBuffer->Unmap(0, nullptr);

        //// 디버그 로그 출력
        //spdlog::debug("[OcclusionCuller] Final Main Count: {} / Max: {}", finalMainCount, m_maxMainCount);

        //// 오버플로우 발생 여부 검사
        //bool isValid = (finalMainCount <= m_maxMainCount);
        //DebugHelper::SuccessCheck(isValid, "Main Buffer Capacity Overflow Detected!");
    }

    if (SUCCEEDED(m_vaseReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVaseCount)))) {
        UINT finalVaseCount = *mappedVaseCount;
        m_vaseReadbackBuffer->Unmap(0, nullptr);

        /*spdlog::debug("[OcclusionCuller] Final Vase Count: {} / Max: {}", finalVaseCount, m_maxVaseCount);

        bool isValid = (finalVaseCount <= m_maxVaseCount);
        DebugHelper::SuccessCheck(isValid, "Vase Buffer Capacity Overflow Detected!");*/
    }
} // ReadbackToCPU

ID3D12Resource* OcclusionCuller::GetFinalMainCommandsBuffer() const { return m_finalMainCommandsBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalVaseCommandsBuffer() const { return m_finalVaseCommandsBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalMainCounterBuffer() const { return m_finalMainCounterBuffer.Get(); }
ID3D12Resource* OcclusionCuller::GetFinalVaseCounterBuffer() const { return m_finalVaseCounterBuffer.Get(); }

void OcclusionCuller::BuildGroupResources(ID3D12Device* device, UINT maxCount, D3D12_RESOURCE_STATES initialState,
    ComPtr<ID3D12Resource>& outVisibleBuf, UINT& outVisibleDescIdx,
    ComPtr<ID3D12Resource>& outCounterBuf, UINT& outCounterDescIdx) {

    UINT commandStructSize = sizeof(GPUCommons::IndirectCommand);
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC visibleDesc = CD3DX12_RESOURCE_DESC::Buffer(
        commandStructSize * maxCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &visibleDesc,
        initialState, nullptr, IID_PPV_ARGS(&outVisibleBuf));

    CD3DX12_RESOURCE_DESC counterDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &counterDesc,
        initialState, nullptr, IID_PPV_ARGS(&outCounterBuf));

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

    // Occlusion 상수 버퍼
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

    BuildGroupResources(device, m_maxMainCount, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        m_finalMainCommandsBuffer, m_finalMainVisibleDescIndex, m_finalMainCounterBuffer, m_finalMainCounterDescIndex);
    BuildGroupResources(device, m_maxVaseCount, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        m_finalVaseCommandsBuffer, m_finalVaseVisibleDescIndex, m_finalVaseCounterBuffer, m_finalVaseCounterDescIndex);

    CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_mainReadbackBuffer));
    device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vaseReadbackBuffer));

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_dummyUAVBuffer));

    m_dummyUAVDescIndex = m_heapAllocator->Allocate();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = 1;
    uavDesc.Buffer.StructureByteStride = 4;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    device->CreateUnorderedAccessView(m_dummyUAVBuffer.Get(), nullptr,
        &uavDesc, m_heapAllocator->GetCPUHandle(m_dummyUAVDescIndex));

    // Phase1 Culled용 버퍼 및 UAV 생성
    BuildGroupResources(device, m_maxMainCount, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        m_phase1CulledMainCommandsBuffer, m_phase1CulledMainUAVIndex, m_phase1CulledMainCounterBuffer, m_phase1CulledMainCounterUAVIndex);
    BuildGroupResources(device, m_maxVaseCount, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        m_phase1CulledVaseCommandsBuffer, m_phase1CulledVaseUAVIndex, m_phase1CulledVaseCounterBuffer, m_phase1CulledVaseCounterUAVIndex);

    // Phase 2에서 위 버퍼들을 읽기 위한 SRV 생성
    auto CreateBufferSRV = [&](ComPtr<ID3D12Resource>& buffer, UINT numElements, UINT stride, UINT& outSrvIdx) {
        outSrvIdx = m_heapAllocator->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = numElements;
        srvDesc.Buffer.StructureByteStride = stride;
        device->CreateShaderResourceView(buffer.Get(), &srvDesc, m_heapAllocator->GetCPUHandle(outSrvIdx));
    }; // CreateBufferSRV

    auto CreateCounterSRV = [&](ComPtr<ID3D12Resource>& buffer, UINT& outSrvIdx) {
        outSrvIdx = m_heapAllocator->Allocate();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = 1;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(buffer.Get(), &srvDesc, m_heapAllocator->GetCPUHandle(outSrvIdx));
    }; // CreateCounterSRV

    UINT cmdStride = sizeof(GPUCommons::IndirectCommand);
    CreateBufferSRV(m_phase1CulledMainCommandsBuffer, m_maxMainCount, cmdStride, m_phase1CulledMainSRVIndex);
    CreateBufferSRV(m_phase1CulledVaseCommandsBuffer, m_maxVaseCount, cmdStride, m_phase1CulledVaseSRVIndex);
    CreateCounterSRV(m_phase1CulledMainCounterBuffer, m_phase1CulledMainCounterSRVIndex);
    CreateCounterSRV(m_phase1CulledVaseCounterBuffer, m_phase1CulledVaseCounterSRVIndex);
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
