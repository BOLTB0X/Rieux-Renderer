#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>

class Frustum;
class DescriptorHeapAllocator;

class FrustumCuller {
public:
    struct InitParams {
        ID3D12Device*            device;
		UINT					 maxMainCount;
		UINT 				     maxVaseCount;
        ID3D12RootSignature*     rootSig;
        ID3D12PipelineState*     pso;
        DescriptorHeapAllocator* heapAllocator;

		InitParams() : device(nullptr),
            maxMainCount(0), maxVaseCount(0),
            rootSig(nullptr), pso(nullptr), heapAllocator(nullptr) {
        }
    }; // InitParams

    struct DispatchParams {
        ID3D12GraphicsCommandList* cmdList;
        ID3D12Resource*            instanceDataBuffer;
        UINT                       instanceDescIndex;
        UINT                       masterIndirectDescriptorIndex;
        ID3D12Resource*            mainIndirectBuffer;
        ID3D12Resource*            vaseIndirectBuffer;

        DispatchParams()
			: cmdList(nullptr), instanceDataBuffer(nullptr), instanceDescIndex(UINT_MAX), masterIndirectDescriptorIndex(UINT_MAX)
            , mainIndirectBuffer(nullptr), vaseIndirectBuffer(nullptr) {
        }
    }; // DispatchParams

public:
    FrustumCuller();
    FrustumCuller(const FrustumCuller&) = delete;
    FrustumCuller& operator=(const FrustumCuller&) = delete;
    ~FrustumCuller();

    bool Init(const InitParams&);
    void Frame(DirectX::XMMATRIX, DirectX::XMMATRIX);
    void Dispatch(const DispatchParams&);
    void ReadbackToCPU(ID3D12GraphicsCommandList*);

public:
    void            OnGUI();
    Frustum* GetFrustum() const;

    ID3D12Resource* GetMainVisibleCommandsBuffer() const;
    ID3D12Resource* GetVaseVisibleCommandsBuffer() const;
    ID3D12Resource* GetMainCounterBuffer()         const;
    ID3D12Resource* GetVaseCounterBuffer()         const;

private:
    void BuildBuffers(ID3D12Device*);
    void BuildGroupResources(ID3D12Device*, UINT,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, UINT&);

private:
    std::unique_ptr<Frustum>               m_frustum;

    ID3D12RootSignature*                   m_rootSignature;
    ID3D12PipelineState*                   m_computePSO;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_frustumConstantBuffer;

    // Main 그룹
    Microsoft::WRL::ComPtr<ID3D12Resource> m_mainVisibleCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_mainCounterBuffer;
    UINT                                   m_mainVisibleDescIndex;
    UINT                                   m_mainCounterDescIndex;

    // Vase 그룹
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vaseVisibleCommandsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vaseCounterBuffer;
    UINT                                   m_vaseVisibleDescIndex;
    UINT                                   m_vaseCounterDescIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_counterResetBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_mainReadbackBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vaseReadbackBuffer;

    UINT                                   m_maxMainCount;
    UINT                                   m_maxVaseCount;
    UINT                                   m_cachedMainVisibleCount;
    UINT                                   m_cachedVaseVisibleCount;
    DescriptorHeapAllocator*               m_heapAllocator;
}; // FrustumCuller