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

    struct DispatchParams {
        ID3D12GraphicsCommandList* cmdList;
        UINT                       hizTextureDescIndex;
        UINT                       frustumMainCommandsDescIndex;
        UINT                       frustumVaseCommandsDescIndex;
        UINT                       frustumMainCountDescIndex;
        UINT                       frustumVaseCountDescIndex;
        UINT                       meshInstanceDataDescIndex;

        DispatchParams()
            : cmdList(nullptr), hizTextureDescIndex(UINT_MAX)
            , frustumMainCommandsDescIndex(UINT_MAX), frustumVaseCommandsDescIndex(UINT_MAX)
            , frustumMainCountDescIndex(UINT_MAX), frustumVaseCountDescIndex(UINT_MAX),
              meshInstanceDataDescIndex(UINT_MAX) {
        }
    }; // DispatchParams

public:
    OcclusionCuller();
    OcclusionCuller(const OcclusionCuller&) = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;
    ~OcclusionCuller();

    bool Init(const InitParams&);
    void Frame(const FrameParams&);

    void Dispatch(const DispatchParams&);
    void ReadbackToCPU(ID3D12GraphicsCommandList*);
    void OnGUI();

public:
    ID3D12Resource* GetFinalMainCommandsBuffer() const;
    ID3D12Resource* GetFinalVaseCommandsBuffer() const;
    ID3D12Resource* GetFinalMainCounterBuffer()  const;
    ID3D12Resource* GetFinalVaseCounterBuffer()  const;

private:
    void BuildBuffers(ID3D12Device*);
    void BuildGroupResources(ID3D12Device*, UINT,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&);

private:
    ID3D12RootSignature*                  m_rootSignature;
    ID3D12PipelineState*                  m_computePSO;

    // ViewProj 및 화면 크기를 셰이더로 넘길 상수 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource> m_occlusionConstantBuffer;

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