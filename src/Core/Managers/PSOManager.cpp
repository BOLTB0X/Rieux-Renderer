#include "Pch.h"
#include "PSOManager.h"
// D3D12
#include "d3dx12.h"
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

    m_shaderBlobs[SharedCommons::CPU_SPONZA_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::CPU_SPONZA_PS_STR] = psBlob;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12PipelineState::InitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = m_mainRootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = m_rtvFormat;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // 버퍼 공통 설정 
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    bool result = true;

    result &= BuildSolidCullBack(SharedCommons::KEY_SPONZA_SOLID_CULL, baseParams);
    result &= BuildSolidCullNone(SharedCommons::KEY_SPONZA_SOLID_NO_CULL, baseParams);
    result &= BuildWireframeCullBack(SharedCommons::KEY_SPONZA_WIRE_CULL, baseParams);
    result &= BuildWireframeCullNone(SharedCommons::KEY_SPONZA_WIRE_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("CPUSponza PSO 변형 초기화 실패");
    }

    return result;
} // BuildStaticSponzaPSO

bool PSOManager::BuildSolidCullBack(const std::string& psoName, D3D12PipelineState::InitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildSolidCullBack

bool PSOManager::BuildSolidCullNone(const std::string& psoName, D3D12PipelineState::InitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildSolidCullNone

bool PSOManager::BuildWireframeCullBack(const std::string& psoName, D3D12PipelineState::InitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildWireframeCullBack

bool PSOManager::BuildWireframeCullNone(const std::string& psoName, D3D12PipelineState::InitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildWireframeCullNone
