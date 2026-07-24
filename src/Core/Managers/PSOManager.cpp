#include "Pch.h"
#include "PSOManager.h"
// D3D12
#include "D3D12/D3D12PipelineState.h"
#include "D3D12/d3dx12.h"
// Uitls
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace Microsoft::WRL;

PSOManager::PSOManager() {
    m_device = nullptr;
    m_mainRootSignature = nullptr;
    m_rtvFormat = DXGI_FORMAT_UNKNOWN;
    m_dsvFormat = DXGI_FORMAT_UNKNOWN;
} // PSOManager

PSOManager::~PSOManager() {
    m_device = nullptr;
    m_mainRootSignature = nullptr;
} // ~PSOManager

PSOManager::InitParams::InitParams()
    : device(nullptr), mainRootSignature(nullptr),
    rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM), dsvFormat(DXGI_FORMAT_D24_UNORM_S8_UINT) {
} // InitParams

bool PSOManager::Init(const InitParams& params) {
    m_device = params.device;
    m_mainRootSignature = params.mainRootSignature;
    m_rtvFormat = params.rtvFormat;
    m_dsvFormat = params.dsvFormat;

    if (!BuildStaticSponzaPSO()) {
        DebugHelper::DebugPrint("Sponza 메인 PSO 빌드 실패");
        return false;
    }

    return true;
} // Init

void PSOManager::Shutdown() {
    m_psoMap.clear();
    m_shaderBlobs.clear();
} // Shutdown

D3D12PipelineState* PSOManager::GetPSO(const std::string& name) const {
    auto it = m_psoMap.find(name);
    if (it != m_psoMap.end()) {
        return it->second.get();
    }
    return nullptr;
} // GetPSO

bool PSOManager::BuildStaticSponzaPSO() {
    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::PBR_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::SPONZA_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::STATIC_SPONZA_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::STATIC_SPONZA_PS_STR] = psBlob;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12PipelineState::InitParams psoParams;
    psoParams.device = m_device;
    psoParams.rootSignature = m_mainRootSignature;
    psoParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    psoParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    psoParams.inputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
    psoParams.numRenderTargets = 1;
    psoParams.rtvFormats[0] = m_rtvFormat;
    psoParams.dsvFormat = m_dsvFormat;
    psoParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoParams.depthStencilState.DepthEnable = TRUE;
    psoParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    auto sponzaPSO = std::make_unique<D3D12PipelineState>();
    if (!sponzaPSO->Init(psoParams)) {
        DebugHelper::DebugPrint("StaticSponza PSO 초기화 실패");
        return false;
    }

    m_psoMap[SharedCommons::STATIC_SPONZA] = std::move(sponzaPSO);
    return true;
} // BuildStaticSponzaPSO