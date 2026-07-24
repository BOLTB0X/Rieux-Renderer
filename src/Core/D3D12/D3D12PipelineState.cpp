#include "Pch.h"
#include "D3D12PipelineState.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

D3D12PipelineState::InitParams::InitParams()
    : device(nullptr),
    rootSignature(nullptr),
    inputLayout{},
    vertexShader{},
    pixelShader{},
    rasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT)),
    blendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT)),
    depthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)),
    numRenderTargets(1),
    rtvFormats{},
    dsvFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
    topologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) {

    // 순수 Z-버퍼 기반 렌더링에 적합하도록 기본 뎁스 옵션 강화
    depthStencilState.DepthEnable = TRUE;
    depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
} // InitParams

bool D3D12PipelineState::Init(const InitParams& params) {
    if (!params.device || !params.rootSignature) {
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = params.rootSignature;
    psoDesc.VS = params.vertexShader;
    psoDesc.PS = params.pixelShader;
    psoDesc.BlendState = params.blendState;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = params.rasterizerState;
    psoDesc.DepthStencilState = params.depthStencilState;
    psoDesc.InputLayout = params.inputLayout;
    psoDesc.PrimitiveTopologyType = params.topologyType;
    psoDesc.NumRenderTargets = params.numRenderTargets;

    for (UINT i = 0; i < params.numRenderTargets; ++i) {
        psoDesc.RTVFormats[i] = params.rtvFormats[i];
    }

    psoDesc.DSVFormat = params.dsvFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = params.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        return false;
    }

    return true;
} // Init

ID3D12PipelineState* D3D12PipelineState::GetPSO() const {
    return m_pipelineState.Get();
} // GetPSO