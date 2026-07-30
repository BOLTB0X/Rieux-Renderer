#pragma once
// Resources
#include "Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

class RenderQueue;

class Sponza : public AssimpModel {
public:
    struct InitParams : public AssimpModel::InitParams {
        ID3D12PipelineState*     psoSolidCull = nullptr;
        ID3D12PipelineState*     psoSolidNoCull = nullptr;
        ID3D12PipelineState*     psoWireCull = nullptr;
        ID3D12PipelineState*     psoWireNoCull = nullptr;
    }; // InitParams

    Sponza();
    virtual ~Sponza();

    bool                     Init(const InitParams&);
    void                     Submit(RenderQueue*);

    const DirectX::XMMATRIX& GetWorldMatrix() const;
    void                     OnGUI();

private:
    DirectX::XMMATRIX         m_worldMatrix;
    ID3D12PipelineState*      m_psoSolidCull;
    ID3D12PipelineState*      m_psoSolidNoCull;
    ID3D12PipelineState*      m_psoWireCull;
    ID3D12PipelineState*      m_psoWireNoCull;
    DescriptorHeapAllocator*  m_heapAllocator;
    bool                      m_enableWireframe;
}; // Sponza