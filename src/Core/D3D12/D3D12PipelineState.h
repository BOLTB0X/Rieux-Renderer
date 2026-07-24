#pragma once
#include <d3d12.h>
#include <wrl.h>

class D3D12PipelineState {
public:
    struct InitParams {
        ID3D12Device*           device;
        ID3D12RootSignature*    rootSignature;
        D3D12_INPUT_LAYOUT_DESC inputLayout;
        // 셰이더 바이트코드
        D3D12_SHADER_BYTECODE    vertexShader;
        D3D12_SHADER_BYTECODE    pixelShader;
        // 상태 설정
        D3D12_RASTERIZER_DESC    rasterizerState;
        D3D12_BLEND_DESC         blendState;
        D3D12_DEPTH_STENCIL_DESC depthStencilState;
        // 포맷
        UINT                          numRenderTargets;
        DXGI_FORMAT                   rtvFormats[8];
        DXGI_FORMAT                   dsvFormat;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType;

        InitParams();
    }; // InitParams

public:
    D3D12PipelineState() = default;
    D3D12PipelineState(const D3D12PipelineState&) = delete;
    D3D12PipelineState& operator=(const D3D12PipelineState&) = delete;
    ~D3D12PipelineState() = default;

    bool Init(const InitParams&);

    ID3D12PipelineState* GetPSO() const;

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
}; // D3D12PipelineState