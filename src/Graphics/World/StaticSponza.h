#pragma once
#include "Resources/Data/AssimpModel.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

class RenderQueue;

class StaticSponza : public AssimpModel {
public:
    StaticSponza();
    virtual ~StaticSponza();

    bool                       Init(const InitParams&);
    void                       Submit(RenderQueue*);
    const DirectX::XMFLOAT4X4& GetWorldMatrix() const;

private:
    bool BuildPSO(ID3D12Device*, ID3D12RootSignature*);

private:
    DirectX::XMFLOAT4X4                         m_worldMatrix;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
}; // StaticSponza