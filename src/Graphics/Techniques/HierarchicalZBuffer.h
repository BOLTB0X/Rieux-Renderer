#pragma once
#include "D3D12RootSignature.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <string>

class RenderTexture;
class D3D12RootSignature;
class D3D12PipelineState;

class HierarchicalZBuffer {
public:
    struct InitParams {
        ID3D12Device*        device;
        ID3D12RootSignature* rootSignature;
        ID3D12PipelineState* pso;

        InitParams() : device(nullptr), rootSignature(nullptr), pso(nullptr) {
		}
    }; // InitParams

    struct BuildParams {
        ID3D12GraphicsCommandList* cmdList;
		RenderTexture*             depthTexture;
        RenderTexture*             hizTexture;

        BuildParams() : cmdList(nullptr), depthTexture(nullptr), hizTexture(nullptr) {
        }
    }; // BuildParams

public:
    HierarchicalZBuffer();
    HierarchicalZBuffer(const HierarchicalZBuffer&) = delete;
    HierarchicalZBuffer& operator=(const HierarchicalZBuffer&) = delete;
    ~HierarchicalZBuffer();

    bool Init(const InitParams&);
    void Build(const BuildParams&);
    void OnHiZ();
    bool IshasValidHiz();

private:

    ID3D12Device*        m_device;
    ID3D12RootSignature* m_rootSignature;
    ID3D12PipelineState* m_pso;
    bool                 m_hasValidHiz;
}; // HierarchicalZBuffer