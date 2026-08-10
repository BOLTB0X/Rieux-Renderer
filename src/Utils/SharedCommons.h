#pragma once
#include <DirectXMath.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

namespace SharedCommons {
    /////////////////////////////////////////
    // 화면
    //////////////////////////////////////////
    static constexpr bool  FULL_SCREEN = false;
    static constexpr bool  VSYNC_ENABLED = true;
    static constexpr int   SCREEN_WIDTH = 800;
    static constexpr int   SCREEN_HEIGHT = 600;
    static constexpr float SCREEN_DEPTH = 8000.0f;
    static constexpr float SCREEN_NEAR = 0.1f;
    static constexpr int   CUBE_MAP_SIZE = 512;
    ////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////
	// 카메라
    static constexpr float MAX_PITCH = 89.0f;
    static constexpr float MIN_PITCH = -89.0f;

    static constexpr float MIN_FOV = 1.0f;
    static constexpr float MAX_FOV = 129.0f;

    static constexpr DirectX::XMFLOAT3 DEFAULT_POSITION = { 0.0f, 0.0f, -5.0f };
    static constexpr DirectX::XMFLOAT3 DEFAULT_ROTATION = { 0.0f, 0.0f, 0.0f };
    static constexpr float DEFAULT_FOV = 60.0f;
	////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
	// 그림자
    static constexpr float SHADOW_VIEW_WIDTH = 2000.0f;
    static constexpr float SHADOW_VIEW_HEIGHT = 2000.0f;
    static constexpr float SHADOW_NEAR_Z = 0.1f;
    static constexpr float SHADOW_FAR_Z = 2000.0f;
    static constexpr float SHADOWMAP_WIDTH = 2048;
    static constexpr float SHADOWMAP_HEIGHT = 2048;
    ///////////////////////////////////////////////////////////////////////////////////////

} // SharedCommons


namespace SharedCommons {
    static constexpr DirectX::XMFLOAT3 LIGHT_DIR = { 0.0f, -1.0f, 0.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_DIFFUSE = { 1.0f, 0.98f, 0.96f, 1.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_AMBIENT = { 0.15f, 0.15f, 0.15f, 1.0f };

} // CB

namespace SharedCommons {
    static const std::wstring DEBUG_LINE_VS = L"HLSL/DebugLineVS.hlsl";
    static const std::wstring DEBUG_LINE_PS = L"HLSL/DebugLinePS.hlsl";

    static const std::wstring PBR_VS = L"HLSL/PBRModelVS.hlsl";
    static const std::wstring CPU_SPONZA_PS = L"HLSL/CPU_SponzaPS.hlsl";
    static const std::wstring SPONZA_FLAT_PS = L"HLSL/FlatSponzaPS.hlsl";

    static const std::string  CPU_SPONZA_VS_STR = "CPUSponzaVS";
    static const std::string  CPU_SPONZA_PS_STR = "CPUSponzaPS";

    static const std::wstring GPU_PBR_VS = L"HLSL/GPU_PBRModelVS.hlsl";
    static const std::wstring GPU_SPONZA_PS = L"HLSL/GPU_SponzaPS.hlsl";

    static const std::string  GPU_SPONZA_VS_STR = "GPUSponzaVS";
    static const std::string  GPU_SPONZA_PS_STR = "GPUSponzaPS";

    static const std::wstring CULLING_CS = L"HLSL/FrustumCullingCS.hlsl";
    static const std::string  CULLING_CS_STR = "FrustumCullingCS";

    static const std::wstring DEPTH_VS = L"HLSL/DepthVS.hlsl";
    static const std::wstring DEPTH_PS = L"HLSL/DepthPS.hlsl";

    static const std::string  DEPTH_VS_STR = "DepthVS";
    static const std::string  DEPTH_PS_STR = "DepthPS";

    static const std::wstring FULLSCREEN_VS = L"HLSL/FullScreenVS.hlsl";
    static const std::wstring LINEAR_Z_TRANS_PS = L"HLSL/LinearZTransPS.hlsl";

    static const std::string  FULLSCREEN_VS_STR = "FullScreenVS";
    static const std::string  LINEAR_Z_TRANS_PS_STR = "LinearZTransPS";

    static const std::wstring HIERARCHICAL_Z_CS = L"HLSL/HierarchicalZCS.hlsl";
    static const std::string  HIERARCHICAL_Z_CS_STR = "HierarchicalZCS";

    static const std::wstring OCCLUSION_CULLING_CS = L"HLSL/OcclusionCullingCS.hlsl";
    static const std::string  OCCLUSION_CULLING_CS_STR = "OcclusionCullingCS";

    inline const std::wstring DEBUG_AABB_VS = L"HLSL/DebugAABBVS.hlsl";

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

    static const std::string  KEY_CULLING_CS = "FrustumCullingCS";
    static const std::string  KEY_CULLING_SIG = "FrustumCullingCS_SIG";

    static const std::string  KEY_GPU_DEPTH_SOLID_CULL = "GPU_DEPTH_SOLID_CULL";
    static const std::string  KEY_GPU_DEPTH_ALPHA_NO_CULL = "GPU_DEPTH_ALPHA_NO_CULL";
    static const std::string  KEY_DEPTH_RECORD_SIG = "DepthRecord_SIG";

    static const std::string  KEY_HIERARCHICAL_Z_SIG = "HierarchicalZ_SIG";
    static const std::string  KEY_HIERARCHICAL_Z_CS_SIG = "HierarchicalZCS_SIG";

    static const std::string  KEY_DEPTH_RENDER_TEXTURE = "DepthRenderTexture";
    static const std::string  KEY_DEPTH_RENDER_TEXTURE_DEBUG = "DepthRenderTexture_DeBug";
    static const std::string  KEY_HIZ_DEPTH_RENDER_TEXTURE = "HiZ_Depth";

    static const std::string  KEY_TRANS_REVERSE_Z_SIG = "Trans_ReverseZ_SIG";
    static const std::string  KEY_TRANS_REVERSE_Z_PSO = "Trans_ReverseZ";

    static const std::string KEY_OCCLUSION_CULLING_SIG = "OcclusionCullingCS_SIG";
    static const std::string KEY_OCCLUSION_CULLING_PSO = "OcclusionCullingCS_PSO";

    inline const std::string KEY_DEBUG_AABB_SIG = "DebugAABBSignature";
    inline const std::string KEY_DEBUG_AABB_PSO = "DebugAABBPso";

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
