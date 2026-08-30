#pragma once
#include <DirectXMath.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <wrl/client.h>

namespace SharedCommons {
    /////////////////////////////////////////
    // 화면
    //////////////////////////////////////////
    static constexpr bool  FULL_SCREEN = false;
    static constexpr bool  VSYNC_ENABLED = true;
    static constexpr int   SCREEN_WIDTH = 800;
    static constexpr int   SCREEN_HEIGHT = 600;
    static constexpr float SCREEN_DEPTH = 4000.0f;
    static constexpr float SCREEN_NEAR = 0.1f;
    static constexpr int   CUBE_MAP_SIZE = 512;
    ////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////
	// 카메라
    static constexpr float MAX_PITCH = 89.0f;
    static constexpr float MIN_PITCH = -89.0f;

    static constexpr float MIN_FOV = 1.0f;
    static constexpr float MAX_FOV = 129.0f;

    static constexpr DirectX::XMFLOAT3 DEFAULT_POSITION = { 0.0f, 10.0f, -5.0f };
    static constexpr DirectX::XMFLOAT3 DEFAULT_ROTATION = { 0.0f, 0.0f, 0.0f };
    static constexpr float DEFAULT_FOV = 60.0f;
	////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
	// 그림자
    static constexpr float SHADOW_VIEW_WIDTH = 1000.0f;
    static constexpr float SHADOW_VIEW_HEIGHT = 1000.0f;
    static constexpr float SHADOW_NEAR_Z = 0.1f;
    static constexpr float SHADOW_FAR_Z = 2000.0f;
    static constexpr float SHADOWMAP_WIDTH = 2048;
    static constexpr float SHADOWMAP_HEIGHT = 2048;
    static constexpr float SHADOW_BIAS = 0.0012f;
    static constexpr float SHADOW_SPREAD = 1.0f;
    static constexpr UINT  CASCADES_COUNT = 4;
    static constexpr float SPLIT_LAMBDA = 0.5f;

    ///////////////////////////////////////////////////////////////////////////////////////

} // SharedCommons

namespace SharedCommons {
    static constexpr DirectX::XMFLOAT3 LIGHT_DIR = { 0.0f, -1.0f, 0.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_DIFFUSE = { 1.0f, 0.98f, 0.96f, 1.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_AMBIENT = { 0.45f, 0.45f, 0.45f, 1.0f };
} // CB

namespace SharedCommons {
    static const std::wstring DEBUG_LINE_VS = L"HLSL/DebugLineVS.hlsl";
    static const std::wstring DEBUG_COLOR_PS = L"HLSL/DebugColorPS.hlsl";

    static const std::wstring PBR_VS = L"HLSL/PBRModelVS.hlsl";
    static const std::wstring CPU_SPONZA_PS = L"HLSL/CPU_SponzaPS.hlsl";
    static const std::wstring SPONZA_FLAT_PS = L"HLSL/FlatSponzaPS.hlsl";

    static const std::string  CPU_SPONZA_VS_STR = "CPUSponzaVS";
    static const std::string  CPU_SPONZA_PS_STR = "CPUSponzaPS";

    static const std::wstring GPU_PBR_VS = L"HLSL/GPU_PBRModelVS.hlsl";
    static const std::wstring GPU_SPONZA_PS = L"HLSL/GPU_SponzaPS.hlsl";

    static const std::string  GPU_SPONZA_VS_STR = "GPUSponzaVS";
    static const std::string  GPU_SPONZA_PS_STR = "GPUSponzaPS";

    static const std::wstring FRUSTUM_CULLING_CS = L"HLSL/FrustumCullingCS.hlsl";
    static const std::string  FRUSTUM_CULLING_CS_STR = "FrustumCullingCS";

    static const std::wstring SHADOW_FRUSTUM_CULLING_CS = L"HLSL/ShadowFrustumCullingCS.hlsl";
    static const std::string  SHADOW_FRUSTUM_CULLING_CS_STR = "ShadowFrustumCullingCS";

    static const std::wstring DEPTH_VS = L"HLSL/DepthVS.hlsl";
    static const std::wstring SHADOW_VS = L"HLSL/ShadowVS.hlsl";
    static const std::wstring DEPTH_PS = L"HLSL/DepthPS.hlsl";

    static const std::string  DEPTH_VS_STR = "DepthVS";
    static const std::string  SHADOW_VS_STR = "ShadowVS";
    static const std::string  DEPTH_PS_STR = "DepthPS";

    static const std::wstring FULLSCREEN_VS = L"HLSL/FullScreenVS.hlsl";
    static const std::wstring LINEAR_Z_TRANS_PS = L"HLSL/LinearZTransPS.hlsl";

    static const std::string  FULLSCREEN_VS_STR = "FullScreenVS";
    static const std::string  LINEAR_Z_TRANS_PS_STR = "LinearZTransPS";

    static const std::wstring HIERARCHICAL_Z_CS = L"HLSL/HierarchicalZCS.hlsl";
    static const std::string  HIERARCHICAL_Z_CS_STR = "HierarchicalZCS";

    static const std::wstring OCCLUSION_CULLING_CS = L"HLSL/OcclusionCullingCS.hlsl";
    static const std::string  OCCLUSION_CULLING_CS_STR = "OcclusionCullingCS";

    static const std::wstring DEBUG_AABB_VS = L"HLSL/DebugAABBVS.hlsl";
    static const std::string  DEBUG_AABB_VS_STR ="DebugAABBVS_str";
    static const std::string  DEBUG_COLOR_PS_STR = "DebugColorPS_str";

    static const std::wstring CSM_VS = L"HLSL/CascadeShadowVS.hlsl";
    static const std::string  CSM_VS_STR = "CSM_VS";

    static const std::wstring PROBE_PS = L"HLSL/GPU_ProbeCapturePS.hlsl";
    static const std::string  PROBE_PS_STR = "PROBE_PS";

    static const std::wstring GBUFFER_PS = L"HLSL/GPU_GBufferPS.hlsl";
    static const std::string  GBUFFER_PS_STR = "GBuffer_PS";

    static const std::wstring PREFILTER_ENVIRONMENT_CS = L"HLSL/PrefilterEnvironmentCS.hlsl";
    static const std::string  PREFILTER_ENVIRONMENT_CS_STR = "PrefilterEnvironmentCS";

    static const std::wstring BRDF_INTEGRATION_CS = L"HLSL/BRDFIntegrationCS.hlsl";
    static const std::string  BRDF_INTEGRATION_CS_STR = "BRDFIntegrationCS";

    static const std::wstring IRRADIANCE_CS = L"HLSL/IrradianceConvolutionCS.hlsl";
    static const std::string  IRRADIANCE_CS_STR = "IrradianceConvolutionCS";

    static const std::wstring DEFERRED_LIGHTING_PS = L"HLSL/GPU_DeferredLightingPS.hlsl";
    static const std::string  DEFERRED_LIGHTING_PS_STR = "GPU_DeferredLightingPS";

    static const std::wstring ACES_FILM_PS = L"HLSL/ACESFilmPS.hlsl";
    static const std::string  ACES_FILM_PS_STR = "ACESFilmPS";

} // HLSL

namespace SharedCommons {
    static const std::string  KEY_DUMMEY_WHITE = "DummeyWhite";
    static const std::string  KEY_DUMMEY_NORMAL = "DummeyNORMAL";

    static const std::string  KEY_CPU_SPONZA = "CPUSponza";
    static const std::string  KEY_CPU_SPONZA_SIG = "CPUSponza_SIG";

    static const std::string  KEY_CPU_SPONZA_SOLID_CULL = "CPUSponza_SOLID_CULL";
    static const std::string  KEY_CPU_SPONZA_SOLID_NO_CULL = "CPUSponza_SOLID_NOCULL";
    static const std::string  KEY_CPU_SPONZA_WIRE_CULL = "CPUSponza_WIRE_CULL";
    static const std::string  KEY_CPU_SPONZA_WIRE_NO_CULL = "CPUSponza_WIRE_NOCULL";

    static const std::string  KEY_GPU_SPONZA = "GPUSponza";
    static const std::string  KEY_GPU_SPONZA_SIG = "GPUSponza_SIG";

    static const std::string  KEY_GPU_SPONZA_SOLID_CULL = "GPUSponza_SOLID_CULL";
    static const std::string  KEY_GPU_SPONZA_SOLID_NO_CULL = "GPUSponza_SOLID_NOCULL";
    static const std::string  KEY_GPU_SPONZA_WIRE_CULL = "GPUSponza_WIRE_CULL";
    static const std::string  KEY_GPU_SPONZA_WIRE_NO_CULL = "GPUSponza_WIRE_NOCULL";

    static const std::string  KEY_FRUSTUM_CULLING_CS = "FrustumCullingCS";
    static const std::string  KEY_SHADOW_FRUSTUM_CULLING_CS = "ShadowFrustumCullingCS";
    static const std::string  KEY_FRUSTUM_CULLING_SIG = "FrustumCullingCS_SIG";
    static const std::string  KEY_SHADOW_FRUSTUM_CULLING_SIG = "ShadowFrustumCullingCS_SIG";

    static const std::string  KEY_GPU_DEPTH_SOLID_CULL = "GPU_DEPTH_SOLID_CULL";
    static const std::string  KEY_GPU_DEPTH_ALPHA_NO_CULL = "GPU_DEPTH_ALPHA_NO_CULL";
    static const std::string  KEY_DEPTH_RECORD_SIG = "DepthRecord_SIG";

    static const std::string  KEY_GPU_SHADOW_SOLID_CULL = "GPU_SHADOW_SOLID_CULL";
    static const std::string  KEY_GPU_SHADOW_ALPHA_NO_CULL = "GPU_SHADOW_ALPHA_NO_CULL";

    static const std::string  KEY_HIERARCHICAL_Z_SIG = "HierarchicalZ_SIG";
    static const std::string  KEY_HIERARCHICAL_Z_CS_SIG = "HierarchicalZCS_SIG";

    static const std::string  KEY_DEPTH_RENDER_TEXTURE = "DepthRenderTexture";
    static const std::string  KEY_DEPTH_RENDER_TEXTURE_DEBUG = "DepthRenderTexture_DeBug";
    static const std::string  KEY_HIZ_DEPTH_RENDER_TEXTURE = "HiZ_Depth";
    static const std::string  KEY_SHADOW_MAP_RENDER_TEXTURE = "ShadowMap_Depth";

    static const std::string  KEY_TRANS_REVERSE_Z_SIG = "Trans_ReverseZ_SIG";
    static const std::string  KEY_TRANS_REVERSE_Z_PSO = "Trans_ReverseZ";

    static const std::string KEY_OCCLUSION_CULLING_SIG = "OcclusionCullingCS_SIG";
    static const std::string KEY_OCCLUSION_CULLING_PSO = "OcclusionCullingCS_PSO";

    static const std::string KEY_BRDF_LUT_RENDER_TEXTURE = "BRDF_LUT";

    static const std::string KEY_DEBUG_AABB_SIG = "DebugAABBSignature";
    static const std::string KEY_DEBUG_AABB_PSO = "DebugAABBPso";
    static const std::string KEY_DEBUG_LINE_SIG = "DebugLineSignature";
    static const std::string KEY_DEBUG_LINE_PSO = "DebugLinePso";

    inline const std::string KEY_GPU_CSM_SOLID_CULL = "GPU_CSM_SOLID_CULL";
    inline const std::string KEY_GPU_CSM_ALPHA_NO_CULL = "GPU_CSM_ALPHA_NO_CULL";

    inline const std::string KEY_GPU_PROBE_SOLID_CULL = "GPU_PROBE_SOLID_CULL";
    inline const std::string KEY_GPU_PROBE_ALPHA_NO_CULL = "GPU_PROBE_ALPHA_NO_CULL";

    static const std::string KEY_GBUFFER0_RENDER_TEXTURE = "GBuffer0_RenderTexture";
    static const std::string KEY_GBUFFER1_RENDER_TEXTURE = "GBuffer1_RenderTexture";

    static const std::string KEY_GBUFFER_SOLID_CULL = "GBuffer_Solid_Cull";
    static const std::string KEY_GBUFFER_ALPHA_NO_CULL = "GBuffer_Alpha_No_Cull";

    static const std::string KEY_BRDF_INTEGRATION_PSO = "BRDF_Integration_PSO";
    static const std::string KEY_PREFILTER_ENVIRONMENT_PSO = "PreFilter_Environment_PSO";
    static const std::string KEY_IRRADIANCE_PSO = "IrradianceConvolutionCS_PSO";

    static const std::string KEY_BRDF_INTEGRATION_SIG = "BRDF_Integration_SIG";
    static const std::string KEY_PREFILTER_ENVIRONMENT_SIG = "PreFilter_Environment_SIG";
    static const std::string KEY_IRRADIANCE_SIG = "IrradianceConvolutionCS_SIG";

    static const std::string KEY_DEFERRED_LIGHTING_PSO = "GPU_DeferredLighting_PSO";
    static const std::string KEY_DEFERRED_LIGHTING_SIG = "GPU_DeferredLighting_SIG";

    static const std::string KEY_TONEMAPPING_SIG = "ToneMapping_PSO";
    static const std::string KEY_TONEMAPPING_PSO = "ToneMapping_SIG";
} // MAP - KEY

namespace SharedCommons {
    static const std::string SPONZA_PATH = "assets/Sponza/sponza.obj";
} // Path

namespace SharedCommons {
    enum class PBRTextureType {
        Albedo, Normal, Metallic,
        Roughness, AO, Alpha,
        Displacement, Emissive,
        Specular, Subsurface,
        Smoothnes, Unknown
    };

    struct PBRTextureKeyword {
        PBRTextureType           type;
        std::vector<std::string> keywords;
    };

    inline const std::vector<PBRTextureKeyword> PBRTEXTURE_KEYWORD_MAP = {
        { PBRTextureType::Albedo,     { "_basecolor", "_albedo", "_alb", "_diffuse", "_col", "_Col", "_diff", "_BaseColor", "_d"}},
        { PBRTextureType::Normal,     { "_normal", "_nrm", "_norm", "_n", "_Normal", "_Nor", "ddr"}},
        { PBRTextureType::Metallic,   { "_metallic", "_metal", "_m" } },
        { PBRTextureType::Roughness,  { "_roughness", "_rough", "_r", "_Roughness", "_Rgh", "_Rgn"}},
        { PBRTextureType::AO,         { "_ao", "_occlusion", "_Occlusion", "_AO"}},
        { PBRTextureType::Alpha,      { "_alpha", "_opacity" , "_Opacity_Map" } },
        { PBRTextureType::Specular,   { "_Specular", "_specular", "_spec" } },
        { PBRTextureType::Subsurface, { "_ssss", "_SSSS", "_sss", "_Subsurface" } },
        { PBRTextureType::Smoothnes,  { "_Smoothness", "_smoothness", "_smoot" } }
    }; // PBRTEXTURE_KEYWORD_MAP

    static constexpr DirectX::XMFLOAT4        ALBEDO_FACTOR = { 1.0f, 1.0f, 1.0f, 1.0f };
    static constexpr float                    METALLIC_FACTOR = 0.0f;
    static constexpr float                    ROUGH_FACTOR = 0.8f;
    static constexpr float                    EMISS_FACTOR = 0.0f;
    static constexpr float                    ALPHA_FACTOR = 0.5f;

} // Assimp Tex
