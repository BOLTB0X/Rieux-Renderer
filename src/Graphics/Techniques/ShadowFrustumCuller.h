#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include <array>

class Frustum;
class DescriptorHeapAllocator;

class ShadowFrustumCuller {
public:
    static const UINT MAX_CASCADES = 4;

    struct InitParams {
        ID3D12Device* device;
        UINT                     maxMainCount; // Main 최대 인스턴스 수
        UINT                     maxVaseCount; // Vase 최대 인스턴스 수
        ID3D12RootSignature* rootSig;
        ID3D12PipelineState* pso;
        DescriptorHeapAllocator* heapAllocator;

        InitParams() : device(nullptr), maxMainCount(0), maxVaseCount(0),
            rootSig(nullptr), pso(nullptr), heapAllocator(nullptr) {
        }
    }; // InitParams

    struct DispatchParams {
        ID3D12GraphicsCommandList* cmdList;
        UINT                       instanceDescIndex;
        UINT                       masterIndirectDescriptorIndex;

        DispatchParams() : cmdList(nullptr), instanceDescIndex(0), masterIndirectDescriptorIndex(0) {
        }
    }; // DispatchParams

public:
    ShadowFrustumCuller();
    ShadowFrustumCuller(const ShadowFrustumCuller&) = delete;
    ShadowFrustumCuller& operator=(const ShadowFrustumCuller&) = delete;
    ~ShadowFrustumCuller();

    bool Init(const InitParams&);

    void Frame(const std::array<DirectX::XMMATRIX, MAX_CASCADES>&, const std::array<DirectX::XMMATRIX, MAX_CASCADES>&);
    void Dispatch(const DispatchParams&);
    void PrepareForIndirectDraw(ID3D12GraphicsCommandList*);
    void RestoreAfterIndirectDraw(ID3D12GraphicsCommandList*);
    void ReadbackToCPU(ID3D12GraphicsCommandList*);
    void OnGUI();

public:
    ID3D12Resource* GetMainVisibleCommandsBuffer(UINT) const;
    ID3D12Resource* GetVaseVisibleCommandsBuffer(UINT) const;

    ID3D12Resource* GetMainCounterBuffer() const;
    ID3D12Resource* GetVaseCounterBuffer() const;

    UINT            GetCounterBufferOffset(UINT) const;

    UINT            GetMainCounterSRVIndex() const;
    UINT            GetVaseCounterSRVIndex() const;

private:
    struct ShadowCullingCB {
        UINT              mainInstances;
        UINT              vaseInstances;
        DirectX::XMFLOAT2 padding;
        DirectX::XMFLOAT4 cascadePlanes[4][6];

        ShadowCullingCB() : mainInstances(0), vaseInstances(0), padding(0.0f, 0.0f), cascadePlanes{} {
        }
    }; // ShadowCullingCB

private:
    void BuildBuffers(ID3D12Device*);
    void BuildGroupResources(ID3D12Device*, UINT, UINT,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        UINT&, UINT&);

private:
    std::array<std::unique_ptr<Frustum>, MAX_CASCADES> m_frustums;
    ID3D12RootSignature*                               m_rootSignature;
    ID3D12PipelineState*                               m_computePSO;

    Microsoft::WRL::ComPtr<ID3D12Resource>             m_shadowConstantBuffer;

    // 4개 캐스케이드용 Main 간접 그리기 명령 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_mainVisibleCommandsCascade[MAX_CASCADES];
    UINT                                               m_mainVisibleDescIndex[MAX_CASCADES];
    UINT                                               m_mainVisibleSRVIndex[MAX_CASCADES];

    // 4개 캐스케이드용 Vase 간접 그리기 명령 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_vaseVisibleCommandsCascade[MAX_CASCADES];
    UINT                                               m_vaseVisibleDescIndex[MAX_CASCADES];
    UINT                                               m_vaseVisibleSRVIndex[MAX_CASCADES];

    // Main 카운터 버퍼 (4개의 UINT 포함)
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_mainCounterBuffer;
    UINT                                               m_mainCounterUAVIndex;
    UINT                                               m_mainCounterSRVIndex;

    // Vase 카운터 버퍼 (4개의 UINT 포함)
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_vaseCounterBuffer;
    UINT                                               m_vaseCounterUAVIndex;
    UINT                                               m_vaseCounterSRVIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource>             m_counterResetBuffer;

    // Readback 버퍼 분리
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_mainReadbackBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_vaseReadbackBuffer;

    UINT                                               m_maxMainCount;
    UINT                                               m_maxVaseCount;

    UINT                                               m_cachedMainVisibleCounts[MAX_CASCADES];
    UINT                                               m_cachedVaseVisibleCounts[MAX_CASCADES];

    DescriptorHeapAllocator*                           m_heapAllocator;
}; // ShadowFrustumCuller