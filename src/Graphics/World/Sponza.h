#pragma once
// Resources
#include "Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

class RenderQueue;
class Frustum;

class Sponza : public AssimpModel {
public:
    struct InitParams : public AssimpModel::InitParams {
        ID3D12RootSignature*       rootSignature = nullptr;
        ID3D12PipelineState*       psoSolidCull = nullptr;
        ID3D12PipelineState*       psoSolidNoCull = nullptr;
        ID3D12PipelineState*       psoWireCull = nullptr;
        ID3D12PipelineState*       psoWireNoCull = nullptr;
    }; // InitParams

public:
    Sponza();
    virtual ~Sponza();

    bool                        Init(const InitParams&);
    void                        Frame(Frustum*);
    void                        Submit(RenderQueue*);
    void                        SubmitIndirect(ID3D12GraphicsCommandList*);

    const DirectX::XMMATRIX&    GetWorldMatrix() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceDataGPUHandle() const;
    UINT                        GetVisibleCount() const;
    void                        OnGUI();

private:
    struct IndirectCommand {
        D3D12_INDEX_BUFFER_VIEW      indexBufferView;
        UINT                         instanceIndex;
        D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;

        IndirectCommand() : indexBufferView{}, instanceIndex(0), drawArgs{} {
        }
    }; // IndirectCommand

private:
    bool BuildInstanceDataBuffer(ID3D12Device*);
    bool BuildIndirectBuffers(ID3D12Device*, ID3D12RootSignature*);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_instanceDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mainIndirectBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_vaseIndirectBuffer;

    D3D12_VERTEX_BUFFER_VIEW                       m_instanceDataSRV;
    DirectX::XMMATRIX                              m_worldMatrix; 
    ID3D12PipelineState*                           m_psoSolidCull;
    ID3D12PipelineState*                           m_psoSolidNoCull;
    ID3D12PipelineState*                           m_psoWireCull;
    ID3D12PipelineState*                           m_psoWireNoCull;
    DescriptorHeapAllocator*                       m_heapAllocator;
    bool                                           m_enableWireframe;
    UINT                                           m_instanceDataDescriptorIndex;
    UINT                                           m_mainIndirectCount;
    UINT                                           m_vaseIndirectCount;
    bool                                           m_freezeCulling;
}; // Sponza