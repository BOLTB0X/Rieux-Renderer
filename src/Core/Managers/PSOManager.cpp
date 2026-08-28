#include "Pch.h"
#include "PSOManager.h"
#include "RendererState.h"
// D3D12
#include "d3dx12.h"
// Uitls
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace Microsoft::WRL;

PSOManager::PSOManager() {
    m_device = nullptr;
    m_rtvFormat = DXGI_FORMAT_UNKNOWN;
    m_dsvFormat = DXGI_FORMAT_UNKNOWN;
} // PSOManager

PSOManager::~PSOManager() {
    m_device = nullptr;
} // ~PSOManager

PSOManager::InitParams::InitParams()
    : device(nullptr), rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM), dsvFormat(DXGI_FORMAT_D24_UNORM_S8_UINT) {
} // InitDefaultParams

bool PSOManager::Init(const InitParams& params) {
    m_device = params.device;
    m_rtvFormat = params.rtvFormat;
    m_dsvFormat = params.dsvFormat;

    if (!BuildGPUDriven(SharedCommons::KEY_GPU_SPONZA_SIG)) {
        DebugHelper::DebugPrint("Sponza 메인 PSO 빌드 실패");
        return false;
    }

    if (!BuildShadowRecord(SharedCommons::KEY_GPU_SPONZA_SIG)) {
        DebugHelper::DebugPrint("Shadow Map PSO 빌드 실패");
        return false;
    }

    if (!BuildCascadedShadowRecord(SharedCommons::KEY_GPU_SPONZA_SIG)) {
        DebugHelper::DebugPrint("Cascaded Shadow Map PSO 빌드 실패");
        return false;
    }

    if (!BuildFrustumCullingCompute(SharedCommons::KEY_FRUSTUM_CULLING_SIG, SharedCommons::FRUSTUM_CULLING_CS)) {
        DebugHelper::DebugPrint("FRUSTUM_CULLING_CS PSO 빌드 실패");
        return false;
	}

    if (!BuildShadowFrustumCullingCompute(SharedCommons::KEY_SHADOW_FRUSTUM_CULLING_SIG, SharedCommons::SHADOW_FRUSTUM_CULLING_CS)) {
        DebugHelper::DebugPrint("FRUSTUM_CULLING_CS PSO 빌드 실패");
        return false;
    }

    if (!BuildDepthRecord(SharedCommons::KEY_GPU_SPONZA_SIG)) {
        DebugHelper::DebugPrint("Depth Record PSO 빌드 실패");
        return false;
	}

    if (!BuildHierarchicalZ(SharedCommons::KEY_HIERARCHICAL_Z_SIG)) {
        DebugHelper::DebugPrint("Hierarchical Z PSO 빌드 실패");
        return false;
    }

    if (!BuildProbeCapture(SharedCommons::KEY_GPU_SPONZA_SIG)) { // 추가
        DebugHelper::DebugPrint("Probe Capture PSO 빌드 실패");
        return false;
    }

    if (!BuildDebugTransReverseZ(SharedCommons::KEY_TRANS_REVERSE_Z_SIG)) {
        DebugHelper::DebugPrint("TransReverseZ Z PSO 빌드 실패");
        return false;
    }

    if (!BuildOcclusionCullingCompute(SharedCommons::KEY_OCCLUSION_CULLING_SIG, SharedCommons::OCCLUSION_CULLING_CS)) {
        DebugHelper::DebugPrint("OcclusionCulling PSO 빌드 실패");
        return false;
    }

    if (!BuildDebugAABB(SharedCommons::KEY_DEBUG_AABB_SIG)) {
        DebugHelper::DebugPrint("BuildDebugAABB PSO 빌드 실패");
        return false;
    }

    if (!BuildDebugLine(SharedCommons::KEY_DEBUG_LINE_SIG)) {
        DebugHelper::DebugPrint("BuildDebugLine PSO 빌드 실패");
        return false;
    }

    if (!BuildGBuffer(SharedCommons::KEY_GPU_SPONZA_SIG)) {
        DebugHelper::DebugPrint("BuildGBuffer PSO 빌드 실패");
        return false;
    }

    if (!BuildBRDFIntegrationCompute(SharedCommons::KEY_BRDF_INTEGRATION_SIG,SharedCommons::BRDF_INTEGRATION_CS)) {
        DebugHelper::DebugPrint("BRDF Integration PSO 빌드 실패");
        return false;
    }

    if (!BuildPrefilterEnvironmentCompute(SharedCommons::KEY_PREFILTER_ENVIRONMENT_SIG,SharedCommons::PREFILTER_ENVIRONMENT_CS)) {
        DebugHelper::DebugPrint("Prefilter Environment PSO 빌드 실패");
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

D3D12RootSignature* PSOManager::GetD3D12RootSignature(const std::string& name) const {
    auto it = m_rootSignatureMap.find(name);
    if (it != m_rootSignatureMap.end()) {
        return it->second.get();
    }
	return nullptr;
} // GetD3D12RootSignature

ID3D12RootSignature* PSOManager::GetID3D12RootSignature(const std::string& name) const {
    auto it = m_rootSignatureMap.find(name);
    return (it != m_rootSignatureMap.end()) ? it->second->GetRootSignature() : nullptr;
} // GetID3D12RootSignature

bool PSOManager::CreateRootSignature(const std::string& name, const std::function<void(RootSignatureBuilder&)>& configure) {
    D3D12RootSignature::InitParams params;
    params.device = m_device;
    configure(params.builder);

    auto rootSig = std::make_unique<D3D12RootSignature>();
    if (!rootSig->Init(params)) {
        DebugHelper::DebugPrint("루트 시그니처 생성 실패: " + name);
        return false;
    }

    m_rootSignatureMap[name] = std::move(rootSig);
    return true;
} // CreateRootSignature

UINT PSOManager::GetRootParamIndex(const std::string& signatureName, const std::string& tag) const {
    auto it = m_rootSignatureMap.find(signatureName);
    if (it == m_rootSignatureMap.end()) {
        DebugHelper::DebugPrint("루트 시그니처를 찾을 수 없음: " + signatureName);
        return UINT_MAX;
    }
    return it->second->GetParamIndex(tag);
} // GetRootParamIndex

bool PSOManager::BuildDefaultSponza(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("FrameCB", 0, D3D12_SHADER_VISIBILITY_ALL)
            .AddCBV("LightCB", 1, D3D12_SHADER_VISIBILITY_ALL)
            .AddConstants("World", 2, 16, D3D12_SHADER_VISIBILITY_VERTEX)
            .AddSRVTable("Albedo", 0, D3D12_SHADER_VISIBILITY_PIXEL)
            .AddSRVTable("Normal", 1, D3D12_SHADER_VISIBILITY_PIXEL)
            .AddSRVTable("Alpha", 2, D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(RendererState::StaticSamplerIndex);
    })) {
        return false;
    }

    RendererState::FrameCBIndex = GetRootParamIndex(signatureKey, "FrameCB");
    RendererState::LightCBIndex = GetRootParamIndex(signatureKey, "LightCB");
    RendererState::WorldIndex = GetRootParamIndex(signatureKey, "World");
    RendererState::Tex0Index = GetRootParamIndex(signatureKey, "Albedo");
    RendererState::Tex1Index = GetRootParamIndex(signatureKey, "Normal");
    RendererState::Tex2Index = GetRootParamIndex(signatureKey, "Alpha");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패: " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::PBR_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::CPU_SPONZA_PS, psBlob.GetAddressOf())) {
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

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = m_rtvFormat;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    bool result = true;

    result &= BuildSolidCullBack(SharedCommons::KEY_CPU_SPONZA_SOLID_CULL, baseParams);
    result &= BuildSolidCullNone(SharedCommons::KEY_CPU_SPONZA_SOLID_NO_CULL, baseParams);
    result &= BuildWireframeCullBack(SharedCommons::KEY_CPU_SPONZA_WIRE_CULL, baseParams);
    result &= BuildWireframeCullNone(SharedCommons::KEY_CPU_SPONZA_WIRE_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("CPUSponza PSO 변형 초기화 실패");
    }

    return result;
} // BuildDefaultSponza

bool PSOManager::BuildGPUDriven(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("FrameCB", 0, D3D12_SHADER_VISIBILITY_ALL)
            .AddCBV("LightCB", 1, D3D12_SHADER_VISIBILITY_ALL)
            .AddConstants("InstanceIndex", 2, 1, D3D12_SHADER_VISIBILITY_ALL)
            .AddConstants("CascadeIndex", 3, 1, D3D12_SHADER_VISIBILITY_VERTEX)
            .AddSRVTable("InstanceData", 1, D3D12_SHADER_VISIBILITY_ALL, 1, 0)
            .AddSRVTable("BindlessTextures", 0, D3D12_SHADER_VISIBILITY_PIXEL, -1, 1)     // t0, space1
            .AddSRVTable("BindlessBuffers", 0, D3D12_SHADER_VISIBILITY_VERTEX, -1, 2)     // t0, space2
            .AddSRVTable("ShadowMap", 2, D3D12_SHADER_VISIBILITY_PIXEL, 1, 0)             // t2, space0
            .AddSRVTable("CSMShadowMap", 3, D3D12_SHADER_VISIBILITY_PIXEL, 1, 0)          // t3
            .AddStaticSampler(RendererState::StaticSamplerIndex)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_SHADER_VISIBILITY_PIXEL);
        })) {
        return false;
    }

    RendererState::FrameCBIndex = GetRootParamIndex(signatureKey, "FrameCB");
    RendererState::LightCBIndex = GetRootParamIndex(signatureKey, "LightCB");
    RendererState::InstanceIndexParam = GetRootParamIndex(signatureKey, "InstanceIndex");
    RendererState::CascadeIndexConstant = GetRootParamIndex(signatureKey, "CascadeIndex");
    RendererState::InstanceDataIndex = GetRootParamIndex(signatureKey, "InstanceData");
    RendererState::BindlessTexIndex = GetRootParamIndex(signatureKey, "BindlessTextures");
    RendererState::BindlessBufIndex = GetRootParamIndex(signatureKey, "BindlessBuffers");
    RendererState::ShadowMapIndex = GetRootParamIndex(signatureKey, "ShadowMap");
    RendererState::CSMShadowMapIndex = GetRootParamIndex(signatureKey, "CSMShadowMap");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::GPU_PBR_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::GPU_SPONZA_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::GPU_SPONZA_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::GPU_SPONZA_PS_STR] = psBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = m_rtvFormat;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    bool result = true;

    result &= BuildSolidCullBack(SharedCommons::KEY_GPU_SPONZA_SOLID_CULL, baseParams);
    result &= BuildSolidCullNone(SharedCommons::KEY_GPU_SPONZA_SOLID_NO_CULL, baseParams);
    result &= BuildWireframeCullBack(SharedCommons::KEY_GPU_SPONZA_WIRE_CULL, baseParams);
    result &= BuildWireframeCullNone(SharedCommons::KEY_GPU_SPONZA_WIRE_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("GPUSponza PSO 변형 초기화 실패");
    }

    return result;
} // BuildGPUDriven

bool PSOManager::BuildFrustumCullingCompute(const std::string& signatureKey, const std::wstring& shaderPath) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("FrustumPlanes", 0, D3D12_SHADER_VISIBILITY_ALL)               // b0
            .AddSRVTable("MasterInstance", 0, D3D12_SHADER_VISIBILITY_ALL)      // t0
            .AddSRVTable("MasterCommands", 1, D3D12_SHADER_VISIBILITY_ALL)      // t1
            .AddUAVTable("VisibleMainCommands", 0, D3D12_SHADER_VISIBILITY_ALL) // u0
            .AddUAVTable("VisibleVaseCommands", 1, D3D12_SHADER_VISIBILITY_ALL) // u1
            .AddUAVTable("MainCount", 2, D3D12_SHADER_VISIBILITY_ALL)           // u2
            .AddUAVTable("VaseCount", 3, D3D12_SHADER_VISIBILITY_ALL);          // u3
        })) {
        return false;
    }

    RendererState::CullingFrustumPlanesIndex = GetRootParamIndex(signatureKey, "FrustumPlanes");
    RendererState::CullingMasterInstanceIndex = GetRootParamIndex(signatureKey, "MasterInstance");
    RendererState::CullingMasterCommandsIndex = GetRootParamIndex(signatureKey, "MasterCommands");
    RendererState::CullingVisibleMainCommandsIndex = GetRootParamIndex(signatureKey, "VisibleMainCommands");
    RendererState::CullingVisibleVaseCommandsIndex = GetRootParamIndex(signatureKey, "VisibleVaseCommands");
    RendererState::CullingMainCountIndex = GetRootParamIndex(signatureKey, "MainCount");
    RendererState::CullingVaseCountIndex = GetRootParamIndex(signatureKey, "VaseCount");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패: " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> csBlob;
    if (!ShaderHelper::InitComputeShader(shaderPath, csBlob.GetAddressOf())) {
        return false;
    }
    m_shaderBlobs[SharedCommons::FRUSTUM_CULLING_CS_STR] = csBlob;

    // Compute PSO 생성
    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader = CD3DX12_SHADER_BYTECODE(csBlob->GetBufferPointer(), csBlob->GetBufferSize());

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint("Culling Compute PSO Init 실패");
        return false;
    }

    m_psoMap[SharedCommons::KEY_FRUSTUM_CULLING_CS] = std::move(pso);
    return true;
} // BuildFrustumCullingCompute

bool PSOManager::BuildShadowFrustumCullingCompute(const std::string& signatureKey, const std::wstring& shaderPath) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("ShadowCullingData", 0, D3D12_SHADER_VISIBILITY_ALL)                  // b0
            .AddSRVTable("MasterInstance", 0, D3D12_SHADER_VISIBILITY_ALL)             // t0
            .AddSRVTable("MasterCommands", 1, D3D12_SHADER_VISIBILITY_ALL)             // t1
            // Main Output (u0 ~ u3)
            .AddUAVTable("MainVisibleCommandsCascade0", 0, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("MainVisibleCommandsCascade1", 1, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("MainVisibleCommandsCascade2", 2, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("MainVisibleCommandsCascade3", 3, D3D12_SHADER_VISIBILITY_ALL)
            // Vase Output (u4 ~ u7)
            .AddUAVTable("VaseVisibleCommandsCascade0", 4, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("VaseVisibleCommandsCascade1", 5, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("VaseVisibleCommandsCascade2", 6, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("VaseVisibleCommandsCascade3", 7, D3D12_SHADER_VISIBILITY_ALL)
            // Counters (u8 ~ u9)
            .AddUAVTable("MainDrawCounts", 8, D3D12_SHADER_VISIBILITY_ALL)
            .AddUAVTable("VaseDrawCounts", 9, D3D12_SHADER_VISIBILITY_ALL);
        })) {
        return false;
    }

    RendererState::ShadowCullingDataIndex = GetRootParamIndex(signatureKey, "ShadowCullingData");
    RendererState::ShadowCullingMasterInstanceIndex = GetRootParamIndex(signatureKey, "MasterInstance");
    RendererState::ShadowCullingMasterCommandsIndex = GetRootParamIndex(signatureKey, "MasterCommands");

    // Main Cascades
    RendererState::ShadowCullingMainVisibleCommandsCascade0Index = GetRootParamIndex(signatureKey, "MainVisibleCommandsCascade0");
    RendererState::ShadowCullingMainVisibleCommandsCascade1Index = GetRootParamIndex(signatureKey, "MainVisibleCommandsCascade1");
    RendererState::ShadowCullingMainVisibleCommandsCascade2Index = GetRootParamIndex(signatureKey, "MainVisibleCommandsCascade2");
    RendererState::ShadowCullingMainVisibleCommandsCascade3Index = GetRootParamIndex(signatureKey, "MainVisibleCommandsCascade3");

    // Vase Cascades
    RendererState::ShadowCullingVaseVisibleCommandsCascade0Index = GetRootParamIndex(signatureKey, "VaseVisibleCommandsCascade0");
    RendererState::ShadowCullingVaseVisibleCommandsCascade1Index = GetRootParamIndex(signatureKey, "VaseVisibleCommandsCascade1");
    RendererState::ShadowCullingVaseVisibleCommandsCascade2Index = GetRootParamIndex(signatureKey, "VaseVisibleCommandsCascade2");
    RendererState::ShadowCullingVaseVisibleCommandsCascade3Index = GetRootParamIndex(signatureKey, "VaseVisibleCommandsCascade3");

    // Counters
    RendererState::ShadowCullingMainDrawCountsIndex = GetRootParamIndex(signatureKey, "MainDrawCounts");
    RendererState::ShadowCullingVaseDrawCountsIndex = GetRootParamIndex(signatureKey, "VaseDrawCounts");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패: " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> csBlob;
    if (!ShaderHelper::InitComputeShader(shaderPath, csBlob.GetAddressOf())) {
        return false;
    }
    m_shaderBlobs[SharedCommons::SHADOW_FRUSTUM_CULLING_CS_STR] = csBlob;

    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader = CD3DX12_SHADER_BYTECODE(csBlob->GetBufferPointer(), csBlob->GetBufferSize());

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint("Shadow Culling Compute PSO Init 실패");
        return false;
    }

    m_psoMap[SharedCommons::KEY_SHADOW_FRUSTUM_CULLING_CS] = std::move(pso);
    return true;
} // BuildShadowFrustumCullingCompute

bool PSOManager::BuildOcclusionCullingCompute(const std::string& signatureKey, const std::wstring& shaderPath) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("OcclusionConstants", 0, D3D12_SHADER_VISIBILITY_ALL)              // b0: ViewProj, ScreenSize
            .AddConstants("PhaseIndex", 1, 2, D3D12_SHADER_VISIBILITY_ALL)          // b1: Phase, Previous Hi-Z Valid
            .AddSRVTable("HiZTexture", 0, D3D12_SHADER_VISIBILITY_ALL)              // t0: Hi-Z 뎁스 텍스처
            .AddSRVTable("FrustumMainCommands", 1, D3D12_SHADER_VISIBILITY_ALL)     // t1: 입력 Main Commands
            .AddSRVTable("FrustumVaseCommands", 2, D3D12_SHADER_VISIBILITY_ALL)     // t2: 입력 Vase Commands
            .AddSRVTable("FrustumMainCount", 3, D3D12_SHADER_VISIBILITY_ALL)        // t3: 입력 Main 개수
            .AddSRVTable("FrustumVaseCount", 4, D3D12_SHADER_VISIBILITY_ALL)        // t4: 입력 Vase 개수
            .AddSRVTable("MeshInstanceData", 5, D3D12_SHADER_VISIBILITY_ALL)        // t5: 인스턴스 데이터
            .AddUAVTable("FinalMainCommands", 0, D3D12_SHADER_VISIBILITY_ALL)       // u0: 최종 통과 Main Commands
            .AddUAVTable("FinalVaseCommands", 1, D3D12_SHADER_VISIBILITY_ALL)       // u1: 최종 통과 Vase Commands
            .AddUAVTable("FinalMainCount", 2, D3D12_SHADER_VISIBILITY_ALL)          // u2: 최종 통과 Main 개수 카운터
            .AddUAVTable("FinalVaseCount", 3, D3D12_SHADER_VISIBILITY_ALL)          // u3: 최종 통과 Vase 개수 카운터

            // Phase 1에서 실패한 녀석들을 보관할 임시 버퍼들
            .AddUAVTable("CulledMainCommands", 4, D3D12_SHADER_VISIBILITY_ALL)      // u4: Phase1 컬링된 Main Commands
            .AddUAVTable("CulledVaseCommands", 5, D3D12_SHADER_VISIBILITY_ALL)      // u5: Phase1 컬링된 Vase Commands
            .AddUAVTable("CulledMainCount", 6, D3D12_SHADER_VISIBILITY_ALL)         // u6: Phase1 컬링된 Main 개수
            .AddUAVTable("CulledVaseCount", 7, D3D12_SHADER_VISIBILITY_ALL)         // u7: Phase1 컬링된 Vase 개수

            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                D3D12_SHADER_VISIBILITY_ALL);                                       // s0: Hi-Z 샘플링용
        })) {
        return false;
    }

    RendererState::OcclusionConstantsIndex = GetRootParamIndex(signatureKey, "OcclusionConstants");
    RendererState::OcclusionPhaseIndex = GetRootParamIndex(signatureKey, "PhaseIndex");
    RendererState::OcclusionHiZTextureIndex = GetRootParamIndex(signatureKey, "HiZTexture");
    RendererState::OcclusionFrustumMainCommandsIndex = GetRootParamIndex(signatureKey, "FrustumMainCommands");
    RendererState::OcclusionFrustumVaseCommandsIndex = GetRootParamIndex(signatureKey, "FrustumVaseCommands");
    RendererState::OcclusionFrustumMainCountIndex = GetRootParamIndex(signatureKey, "FrustumMainCount");
    RendererState::OcclusionFrustumVaseCountIndex = GetRootParamIndex(signatureKey, "FrustumVaseCount");
    RendererState::OcclusionMeshInstanceDataIndex = GetRootParamIndex(signatureKey, "MeshInstanceData");

    RendererState::OcclusionFinalMainCommandsIndex = GetRootParamIndex(signatureKey, "FinalMainCommands");
    RendererState::OcclusionFinalVaseCommandsIndex = GetRootParamIndex(signatureKey, "FinalVaseCommands");
    RendererState::OcclusionFinalMainCountIndex = GetRootParamIndex(signatureKey, "FinalMainCount");
    RendererState::OcclusionFinalVaseCountIndex = GetRootParamIndex(signatureKey, "FinalVaseCount");

    RendererState::OcclusionCulledMainCommandsIndex = GetRootParamIndex(signatureKey, "CulledMainCommands");
    RendererState::OcclusionCulledVaseCommandsIndex = GetRootParamIndex(signatureKey, "CulledVaseCommands");
    RendererState::OcclusionCulledMainCountIndex = GetRootParamIndex(signatureKey, "CulledMainCount");
    RendererState::OcclusionCulledVaseCountIndex = GetRootParamIndex(signatureKey, "CulledVaseCount");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (Occlusion Culling): " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> csBlob;
    if (!ShaderHelper::InitComputeShader(shaderPath, csBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::OCCLUSION_CULLING_CS_STR] = csBlob;

    // Compute PSO 생성
    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader = CD3DX12_SHADER_BYTECODE(csBlob->GetBufferPointer(), csBlob->GetBufferSize());

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint("Occlusion Culling Compute PSO Init 실패");
        return false;
    }

    m_psoMap[SharedCommons::KEY_OCCLUSION_CULLING_PSO] = std::move(pso);
    return true;
} // BuildOcclusionCullingCompute

bool PSOManager::BuildBRDFIntegrationCompute(const std::string& signatureKey, const std::wstring& shaderPath) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("BRDF_LUT_CB", 0, D3D12_SHADER_VISIBILITY_ALL) // b0
            .AddUAVTable("BRDFLUT", 0, D3D12_SHADER_VISIBILITY_ALL); // u0
        })) {
        return false;
    }

    RendererState::BRDFLUTConstantBufferIndex = GetRootParamIndex(signatureKey, "BRDF_LUT_CB");
    RendererState::BRDFLUTIndex = GetRootParamIndex(signatureKey, "BRDFLUT");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (BRDF Integration): " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> csBlob;
    if (!ShaderHelper::InitComputeShader(shaderPath, csBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::BRDF_INTEGRATION_CS_STR] = csBlob;

    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader =
        CD3DX12_SHADER_BYTECODE(
            csBlob->GetBufferPointer(),
            csBlob->GetBufferSize());

    auto pso = std::make_unique<D3D12PipelineState>();

    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint("BRDF Integration Compute PSO Init 실패");
        return false;
    }

    m_psoMap[SharedCommons::KEY_BRDF_INTEGRATION_PSO] = std::move(pso);

    return true;
} // BuildBRDFIntegrationCompute

bool PSOManager::BuildPrefilterEnvironmentCompute(const std::string& signatureKey, const std::wstring& shaderPath) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddConstants("PrefilterCB", /*레지스터*/ 0, /*32bit 값 개수*/ 4, D3D12_SHADER_VISIBILITY_ALL)
            .AddSRVTable("SourceCubemap", 0, D3D12_SHADER_VISIBILITY_ALL) // t0
            .AddUAVTable("OutputMipFace", 0, D3D12_SHADER_VISIBILITY_ALL) // u0
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                D3D12_SHADER_VISIBILITY_ALL);
        })) {
        return false;
    }

    RendererState::PrefilterConstantBufferIndex = GetRootParamIndex(signatureKey, "PrefilterCB");
    RendererState::SourceCubemapIndex = GetRootParamIndex(signatureKey, "SourceCubemap");
    RendererState::OutputMipFaceIndex = GetRootParamIndex(signatureKey, "OutputMipFace");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);

    if (!rootSignature) {
        DebugHelper::DebugPrint(
            "루트 시그니처 조회 실패 (Prefilter Environment): " +
            signatureKey);

        return false;
    }

    ComPtr<IDxcBlob> csBlob;

    if (!ShaderHelper::InitComputeShader(
        shaderPath,
        csBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::PREFILTER_ENVIRONMENT_CS_STR] = csBlob;

    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader =CD3DX12_SHADER_BYTECODE(
            csBlob->GetBufferPointer(),
            csBlob->GetBufferSize());

    auto pso = std::make_unique<D3D12PipelineState>();

    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint(
            "Prefilter Environment Compute PSO Init 실패");

        return false;
    }

    m_psoMap[SharedCommons::KEY_PREFILTER_ENVIRONMENT_PSO] =
        std::move(pso);

    return true;
} // BuildPrefilterEnvironmentCompute

bool PSOManager::BuildDepthRecord(const std::string& signatureKey) {
    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (DepthRecord PSO): " + signatureKey);
        return false;
    }

    // Depth 전용 셰이더 로드
    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::DEPTH_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::DEPTH_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::DEPTH_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::DEPTH_PS_STR] = psBlob;

    // Base PSO 파라미터
    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 0;
    baseParams.rtvFormats[0] = DXGI_FORMAT_UNKNOWN;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    bool result = true;

    baseParams.pixelShader = { nullptr, 0 };
    result &= BuildSolidCullBack(SharedCommons::KEY_GPU_DEPTH_SOLID_CULL, baseParams);

    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    result &= BuildSolidCullNone(SharedCommons::KEY_GPU_DEPTH_ALPHA_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("GPU DepthRecord PSO 변형 초기화 실패");
    }

    return result;
} // BuildDepthRecord

bool PSOManager::BuildShadowRecord(const std::string& signatureKey) {
    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (ShadowRecord PSO): " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::SHADOW_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::DEPTH_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::SHADOW_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::DEPTH_PS_STR] = psBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 0;
    baseParams.rtvFormats[0] = DXGI_FORMAT_UNKNOWN;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    baseParams.rasterizerState.DepthBias = 100;
    baseParams.rasterizerState.SlopeScaledDepthBias = 1.0f;
    baseParams.rasterizerState.DepthBiasClamp = 0.0f;

    bool result = true;

    baseParams.pixelShader = { nullptr, 0 };
    result &= BuildSolidCullBack(SharedCommons::KEY_GPU_SHADOW_SOLID_CULL, baseParams);

    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    result &= BuildSolidCullNone(SharedCommons::KEY_GPU_SHADOW_ALPHA_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("GPU ShadowRecord PSO 변형 초기화 실패");
    }

    return result;
} // BuildShadowRecord

bool PSOManager::BuildCascadedShadowRecord(const std::string& signatureKey) {
    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (CascadedShadowRecord PSO): " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::CSM_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::DEPTH_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::CSM_VS_STR] = vsBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 0;
    baseParams.rtvFormats[0] = DXGI_FORMAT_UNKNOWN;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    baseParams.rasterizerState.DepthBias = 100;
    baseParams.rasterizerState.SlopeScaledDepthBias = 1.0f;
    baseParams.rasterizerState.DepthBiasClamp = 0.0f;

    bool result = true;

    baseParams.pixelShader = { nullptr, 0 };
    result &= BuildSolidCullBack(SharedCommons::KEY_GPU_CSM_SOLID_CULL, baseParams);

    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    result &= BuildSolidCullNone(SharedCommons::KEY_GPU_CSM_ALPHA_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("GPU CascadedShadowRecord PSO 변형 초기화 실패");
    }

    return result;
} // BuildCascadedShadowRecord

bool PSOManager::BuildHierarchicalZ(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddConstants("HZBConstants", 0, 2, D3D12_SHADER_VISIBILITY_ALL) // 2 = uint2 InputResolution
            .AddSRVTable("DepthTexture", 0, D3D12_SHADER_VISIBILITY_ALL) // t0
            .AddUAVTable("HZBTexture", 0, D3D12_SHADER_VISIBILITY_ALL) // u0
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE);
        })) {
        return false;
    }
    RendererState::HZBConstantsIndex = GetRootParamIndex(signatureKey, "HZBConstants");
    RendererState::DepthTextureIndex = GetRootParamIndex(signatureKey, "DepthTexture");
    RendererState::HZBTextureIndex = GetRootParamIndex(signatureKey, "HZBTexture");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (Hierarchical Z PSO): " + signatureKey);
        return false;
    }
    ComPtr<IDxcBlob> csBlob;
    if (!ShaderHelper::InitComputeShader(SharedCommons::HIERARCHICAL_Z_CS, csBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::HIERARCHICAL_Z_CS_STR] = csBlob;
    D3D12PipelineState::ComputeInitParams computeParams;
    computeParams.device = m_device;
    computeParams.rootSignature = rootSignature;
    computeParams.computeShader = CD3DX12_SHADER_BYTECODE(csBlob->GetBufferPointer(), csBlob->GetBufferSize());
    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->InitCompute(computeParams)) {
        DebugHelper::DebugPrint("Hierarchical Z Compute PSO Init 실패");
        return false;
    }

    m_psoMap[SharedCommons::KEY_HIERARCHICAL_Z_CS_SIG] = std::move(pso);
    return true;
} // BuildHierarchicalZ

bool PSOManager::BuildProbeCapture(const std::string& signatureKey) {
    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        DebugHelper::DebugPrint("루트 시그니처 조회 실패 (ProbeCapture PSO): " + signatureKey);
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::GPU_PBR_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::PROBE_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::PROBE_PS_STR] = psBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    bool result = true;
    result &= BuildSolidCullBack(SharedCommons::KEY_GPU_PROBE_SOLID_CULL, baseParams);
    result &= BuildSolidCullNone(SharedCommons::KEY_GPU_PROBE_ALPHA_NO_CULL, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("GPU ProbeCapture PSO 변형 초기화 실패");
    }

    return result;
} // BuildProbeCapture

bool PSOManager::BuildGBuffer(const std::string& signatureKey) {
    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) return false;

    ComPtr<IDxcBlob> vsBlob, psBlob;
    if (!ShaderHelper::InitVertexShader(SharedCommons::GPU_PBR_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::GBUFFER_PS, psBlob.GetAddressOf())) {
        return false;
    }
    m_shaderBlobs[SharedCommons::GBUFFER_PS_STR] = psBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 2; // MRT
    baseParams.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    baseParams.rtvFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 쓰기 안 함 - Phase1 결과 재사용
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;      // Phase1 깊이와 일치하는 픽셀만

    bool result = true;
    result &= BuildSolidCullBack(SharedCommons::KEY_GBUFFER_SOLID_CULL, baseParams);
    result &= BuildSolidCullNone(SharedCommons::KEY_GBUFFER_ALPHA_NO_CULL, baseParams);
    return result;
} // BuildGBuffer

bool PSOManager::BuildDebugTransReverseZ(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddConstants("CameraClipCB", 0, 2, D3D12_SHADER_VISIBILITY_PIXEL)      // b0: g_Near, g_Far (32-bit 값 2개)
            .AddSRVTable("DepthTexture", 0, D3D12_SHADER_VISIBILITY_PIXEL, 1, 0) // t0, space0: 원본 뎁스 텍스처
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                D3D12_SHADER_VISIBILITY_PIXEL);                                   // s0: Clamp 샘플러
        })) {
        return false;
    }

     RendererState::DebugCameraClipIndex = GetRootParamIndex(signatureKey, "CameraClipCB");
     RendererState::DebugDepthTexIndex   = GetRootParamIndex(signatureKey, "DepthTexture");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        return false;
    }

    ComPtr<IDxcBlob> vsBlob;
    ComPtr<IDxcBlob> psBlob;

    if (!ShaderHelper::InitVertexShader(SharedCommons::FULLSCREEN_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::LINEAR_Z_TRANS_PS, psBlob.GetAddressOf())) {
        return false;
    }

    m_shaderBlobs[SharedCommons::FULLSCREEN_VS_STR] = vsBlob;
    m_shaderBlobs[SharedCommons::LINEAR_Z_TRANS_PS_STR] = psBlob;

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());

    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    baseParams.dsvFormat = DXGI_FORMAT_UNKNOWN;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    baseParams.depthStencilState.DepthEnable = FALSE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    bool result = true;
    result &= BuildSolidCullNone(SharedCommons::KEY_TRANS_REVERSE_Z_PSO, baseParams);

    if (!result) {
        DebugHelper::DebugPrint("TransReverseZ PSO 초기화 실패");
    }

    return result;
} // BuildDebugTransReverseZ

bool PSOManager::BuildDebugAABB(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("FrameCB", 0, D3D12_SHADER_VISIBILITY_ALL)                          // b0: ViewProj 행렬
            .AddConstants("Debug_InstanceIndex", 2, 2, D3D12_SHADER_VISIBILITY_ALL)  // b2: InstanceIndex
            .AddSRVTable("InstanceData", 1, D3D12_SHADER_VISIBILITY_ALL, 1, 0);      // t1 space0
        })) {
        return false;
    }

    RendererState::DebugFrameIndex = GetRootParamIndex(signatureKey, "FrameCB");
    RendererState::DebugInstanceIndex = GetRootParamIndex(signatureKey, "Debug_InstanceIndex");
    RendererState::DebugInstanceDataIndex = GetRootParamIndex(signatureKey, "InstanceData");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        return false;
    }

    ComPtr<IDxcBlob> vsBlob, psBlob;
    if (!ShaderHelper::InitVertexShader(SharedCommons::DEBUG_AABB_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::DEBUG_COLOR_PS, psBlob.GetAddressOf())) {
        return false;
    }

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { nullptr, 0 };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = m_rtvFormat;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    baseParams.rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    baseParams.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    baseParams.depthStencilState.DepthEnable = true;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; // Reverse-Z

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(baseParams)) return false;

    m_psoMap[SharedCommons::KEY_DEBUG_AABB_PSO] = std::move(pso);
    return true;
} // BuildDebugAABB

bool PSOManager::BuildDebugLine(const std::string& signatureKey) {
    if (!CreateRootSignature(signatureKey, [](RootSignatureBuilder& b) {
        b.AddCBV("FrameCB", 0, D3D12_SHADER_VISIBILITY_VERTEX);
        })) {
        return false;
    }

    RendererState::DebugLineFrameIndex = GetRootParamIndex(signatureKey, "FrameCB");

    ID3D12RootSignature* rootSignature = GetID3D12RootSignature(signatureKey);
    if (!rootSignature) {
        return false;
    }

    ComPtr<IDxcBlob> vsBlob, psBlob;
    if (!ShaderHelper::InitVertexShader(SharedCommons::DEBUG_LINE_VS, vsBlob.GetAddressOf()) ||
        !ShaderHelper::InitPixelShader(SharedCommons::DEBUG_COLOR_PS, psBlob.GetAddressOf())) {
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12PipelineState::DefaultInitParams baseParams;
    baseParams.device = m_device;
    baseParams.rootSignature = rootSignature;
    baseParams.vertexShader = CD3DX12_SHADER_BYTECODE(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
    baseParams.pixelShader = CD3DX12_SHADER_BYTECODE(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
    baseParams.inputLayout = { inputLayout, static_cast<UINT>(_countof(inputLayout)) };
    baseParams.numRenderTargets = 1;
    baseParams.rtvFormats[0] = m_rtvFormat;
    baseParams.dsvFormat = m_dsvFormat;
    baseParams.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    baseParams.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    baseParams.rasterizerState.AntialiasedLineEnable = TRUE;
    baseParams.depthStencilState.DepthEnable = TRUE;
    baseParams.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    baseParams.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(baseParams)) {
        return false;
    }

    m_psoMap[SharedCommons::KEY_DEBUG_LINE_PSO] = std::move(pso);
    return true;
} // BuildDebugLine

bool PSOManager::BuildSolidCullBack(const std::string& psoName, D3D12PipelineState::DefaultInitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildSolidCullBack

bool PSOManager::BuildSolidCullNone(const std::string& psoName, D3D12PipelineState::DefaultInitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildSolidCullNone

bool PSOManager::BuildWireframeCullBack(const std::string& psoName, D3D12PipelineState::DefaultInitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildWireframeCullBack

bool PSOManager::BuildWireframeCullNone(const std::string& psoName, D3D12PipelineState::DefaultInitParams params) {
    params.rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    params.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    auto pso = std::make_unique<D3D12PipelineState>();
    if (!pso->Init(params)) return false;

    m_psoMap[psoName] = std::move(pso);
    return true;
} // BuildWireframeCullNone
