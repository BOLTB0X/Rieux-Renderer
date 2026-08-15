#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <DirectXMath.h>

class DescriptorHeapAllocator;

class OcclusionCuller {
public:
    struct InitParams {
        ID3D12Device* device;
        UINT                     maxMainCount;
        UINT                     maxVaseCount;
        ID3D12RootSignature*     rootSig;
        ID3D12PipelineState*     pso;
        DescriptorHeapAllocator* heapAllocator;

        InitParams() : device(nullptr),
            maxMainCount(0), maxVaseCount(0),
            rootSig(nullptr), pso(nullptr), heapAllocator(nullptr) {
        }
    }; // InitParams

    struct FrameParams {
        DirectX::XMMATRIX viewMatrix;
        DirectX::XMMATRIX projectionMatrix;
        float             screenWidth;
        float             screenHeight;

        FrameParams() : viewMatrix(DirectX::XMMatrixIdentity()), projectionMatrix(DirectX::XMMatrixIdentity()),
            screenWidth(0.0f), screenHeight(0.0f) {
        }
    }; // FrameParams

    struct DispatchPhase1Params {
        ID3D12GraphicsCommandList* cmdList;
        UINT                       previousHizTextureDescIndex;  // N-1 프레임 HiZ
        UINT                       frustumMainCommandsDescIndex;
        UINT                       frustumVaseCommandsDescIndex;
        UINT                       frustumMainCountDescIndex;
        UINT                       frustumVaseCountDescIndex;
        UINT                       meshInstanceDataDescIndex;
        bool                       hasPreviousHiz;

        DispatchPhase1Params() : cmdList(nullptr), previousHizTextureDescIndex(0),
            frustumMainCommandsDescIndex(0), frustumVaseCommandsDescIndex(0),
            frustumMainCountDescIndex(0), frustumVaseCountDescIndex(0), meshInstanceDataDescIndex(0),
            hasPreviousHiz(false) {
        }
    }; // DispatchPhase1Params

    struct DispatchPhase2Params {
        ID3D12GraphicsCommandList* cmdList;
        UINT                       currentHizTextureDescIndex;   // N 프레임(방금 만든) HiZ
        UINT                       meshInstanceDataDescIndex;

        DispatchPhase2Params() : cmdList(nullptr),
            currentHizTextureDescIndex(0), meshInstanceDataDescIndex(0) {
        }
    }; // DispatchPhase2Params

public:
    OcclusionCuller();
    OcclusionCuller(const OcclusionCuller&) = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;
    ~OcclusionCuller();

    bool Init(const InitParams&);
    void Frame(const FrameParams&);

    void DispatchPhase1(const DispatchPhase1Params&);
    void DispatchPhase2(const DispatchPhase2Params&);
    void ReadbackToCPU(ID3D12GraphicsCommandList*);
    void OnGUI();

public:
    ID3D12Resource* GetFinalMainCommandsBuffer() const;
    ID3D12Resource* GetFinalVaseCommandsBuffer() const;
    ID3D12Resource* GetFinalMainCounterBuffer()  const;
    ID3D12Resource* GetFinalVaseCounterBuffer()  const;

private:
    void BuildBuffers(ID3D12Device*);
    void BuildGroupResources(ID3D12Device*, UINT, D3D12_RESOURCE_STATES,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&);

private:
    ID3D12RootSignature*                  m_rootSignature;
    ID3D12PipelineState*                  m_computePSO;

    // ViewProj 및 화면 크기를 셰이더로 넘길 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource> m_occlusionConstantBuffer;

    // Phase 1에서 실패한(Culled) 물체들을 담아둘 임시 버퍼 (Phase 2의 입력이 됨)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_phase1CulledMainCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_phase1CulledMainCounterBuffer;
    UINT                                   m_phase1CulledMainUAVIndex;
    UINT                                   m_phase1CulledMainCounterUAVIndex;
    UINT                                   m_phase1CulledMainSRVIndex;        // Phase 2에서 읽기 위한 SRV
    UINT                                   m_phase1CulledMainCounterSRVIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_phase1CulledVaseCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_phase1CulledVaseCounterBuffer;
    UINT                                   m_phase1CulledVaseUAVIndex;
    UINT                                   m_phase1CulledVaseCounterUAVIndex;
    UINT                                   m_phase1CulledVaseSRVIndex;        // Phase 2에서 읽기 위한 SRV
    UINT                                   m_phase1CulledVaseCounterSRVIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_dummyUAVBuffer;
    UINT                                   m_dummyUAVDescIndex;

    // Main 그룹 (최종 생존자)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_finalMainCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_finalMainCounterBuffer;
    UINT                                   m_finalMainVisibleDescIndex;
    UINT                                   m_finalMainCounterDescIndex;

    // Vase 그룹 (최종 생존자)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_finalVaseCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_finalVaseCounterBuffer;
    UINT                                   m_finalVaseVisibleDescIndex;
    UINT                                   m_finalVaseCounterDescIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_counterResetBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_mainReadbackBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vaseReadbackBuffer;

    UINT                                   m_maxMainCount;
    UINT                                   m_maxVaseCount;
    UINT                                   m_cachedMainVisibleCount;
    UINT                                   m_cachedVaseVisibleCount;

    DescriptorHeapAllocator*               m_heapAllocator;
}; // OcclusionCuller
