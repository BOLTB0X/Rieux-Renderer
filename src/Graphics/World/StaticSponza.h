#pragma once
// Resources
#include "Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

class RenderQueue;
class DescriptorHeapAllocator;

class StaticSponza : public AssimpModel {
public:
    struct InitParams : public AssimpModel::InitParams {
        ID3D12PipelineState*     pso = nullptr;
        DescriptorHeapAllocator* heapAllocator = nullptr;
    }; // InitParams

    StaticSponza();
    virtual ~StaticSponza();

    bool                     Init(const InitParams&);
    void                     Submit(RenderQueue*);
    const DirectX::XMMATRIX& GetWorldMatrix() const;

private:
    DirectX::XMMATRIX         m_worldMatrix;
    ID3D12PipelineState*      m_pso;
    DescriptorHeapAllocator*  m_heapAllocator;
}; // StaticSponza