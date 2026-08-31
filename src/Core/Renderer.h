#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <string>
#include <memory>
#include "RendererState.h"

class D3D12Device;
class CommandQueue;
class D3D12SwapChain;
class D3D12RootSignature;
class D3D12PipelineState;
class RenderTarget;
class PSOManager;
class RenderQueue;
class Camera;
class FrustumCuller;
class HierarchicalZBuffer;
class OcclusionCuller;
class DirectionalLight;
class GPUMonitor;
class TextureManager;
class DescriptorHeapAllocator;
class ImGuiManager;
class RenderTexture;
class RenderTextureManager;
class Sponza;
class RendererDebugger;
class ShadowFrustumCuller;
class CascadedShadowMap;
class EnvironmentProbe;
class BRDFIntegrationLUT;
class PrefilterEnvironment;
class IrradianceConvolution;

class Renderer {
public:
    using FrameParams = RendererState::RuntimeFrameParams;

    struct InitParams {
        HWND                          hwnd;
        std::shared_ptr<ImGuiManager> imGuiManager;

        InitParams() : hwnd(nullptr), imGuiManager(nullptr) {
        }
    }; // InitDefaultParams

    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer();

    bool Init(const InitParams&);
    bool Frame(const FrameParams&);
    void Shutdown();

private:
    enum GPUQueryIndex : uint32_t {
        GPU_QUERY_PROBE_BEGIN = 0,
        GPU_QUERY_PROBE_END,
        GPU_QUERY_FRUSTUM_BEGIN,
        GPU_QUERY_FRUSTUM_END,
        GPU_QUERY_SHADOW_FRUSTUM_BEGIN,
        GPU_QUERY_SHADOW_FRUSTUM_END,
        GPU_QUERY_OCCLUSION_PHASE1_BEGIN,
        GPU_QUERY_OCCLUSION_PHASE1_END,
        GPU_QUERY_DEPTH_BEGIN,
        GPU_QUERY_DEPTH_END,
        GPU_QUERY_HIZ_BEGIN,
        GPU_QUERY_HIZ_END,
        GPU_QUERY_CSM_BEGIN,
        GPU_QUERY_CSM_END,
        GPU_QUERY_OCCLUSION_PHASE2_BEGIN,
        GPU_QUERY_OCCLUSION_PHASE2_END,
        GPU_QUERY_GBUFFER_BEGIN,
        GPU_QUERY_GBUFFER_END,
        GPU_QUERY_SCENE_BEGIN,
        GPU_QUERY_SCENE_END,
        GPU_QUERY_SSR_BEGIN,
        GPU_QUERY_SSR_END,
        GPU_QUERY_IMGUI_BEGIN,
        GPU_QUERY_IMGUI_END,
        GPU_QUERY_COUNT
    };

private:
    bool Render();
    bool LoadPipeline(HWND, int, int);
    bool LoadSceneRenderTarget(int, int);
    bool LoadAssets(HWND);
    bool LoadGUIs(HWND, std::shared_ptr<ImGuiManager>);

private:
    void PopulateCommandList();

    void ProbeCapturePass(ID3D12GraphicsCommandList*);
    void FrustumPass(ID3D12GraphicsCommandList*);
    void ShadowFrustumPass(ID3D12GraphicsCommandList*);
    void DepthPass(ID3D12GraphicsCommandList*);
    void ShadowPass(ID3D12GraphicsCommandList*);
    void OcclusionPhase1Pass(ID3D12GraphicsCommandList*);
    void OcclusionPhase2Pass(ID3D12GraphicsCommandList*);
    void SponzaPass(ID3D12GraphicsCommandList*);
    void GBufferPass(ID3D12GraphicsCommandList*);
    void BRDFIntegrationPass(ID3D12GraphicsCommandList*);
    void DeferredLightingPass(ID3D12GraphicsCommandList*);
    void ScreenSpaceReflectionPass(ID3D12GraphicsCommandList*);
    void ToneMappingPass(ID3D12GraphicsCommandList*);

    void OnGUI();

private:
    // --------------------------------------------------
    // 파이프라인 객체
    // --------------------------------------------------
    std::unique_ptr<D3D12Device>             m_D3D12Device;
    std::unique_ptr<CommandQueue>            m_CommandQueue;
    std::unique_ptr<D3D12SwapChain>          m_SwapChain;
    std::unique_ptr<DescriptorHeapAllocator> m_sharedDescriptorAllocator;
    std::unique_ptr<RenderTextureManager>    m_RenderTextureManager;
    std::unique_ptr<RenderTarget>            m_SceneRenderTarget;
    std::shared_ptr<PSOManager>              m_PSOManager;
    // --------------------------------------------------
    // 공용 데이터
    // --------------------------------------------------
    std::unique_ptr<RendererState>           m_RendererState;
    std::unique_ptr<RendererDebugger>        m_RendererDebugger;
    std::unique_ptr<RenderQueue>             m_RenderQueue;
    std::unique_ptr<Camera>                  m_SceneCamera;
    std::unique_ptr<Camera>                  m_MasterCamera;
    std::unique_ptr<DirectionalLight>        m_DirectionalLight;
    std::unique_ptr<GPUMonitor>              m_GPUMonitor;
    std::shared_ptr<TextureManager>          m_TextureManager;
    std::shared_ptr<ImGuiManager>            m_ImGuiManager;
    // --------------------------------------------------
    // Techniques 데이터
    // --------------------------------------------------
    std::unique_ptr<FrustumCuller>           m_FrustumCuller;
    std::unique_ptr<ShadowFrustumCuller>     m_ShadowFrustumCuller;
    std::unique_ptr<HierarchicalZBuffer>     m_HierarchicalZBuffer;
    std::unique_ptr<OcclusionCuller>         m_OcclusionCuller;
    std::unique_ptr<CascadedShadowMap>       m_CascadedShadowMap;
    std::unique_ptr<EnvironmentProbe>        m_EnvironmentProbe;
    std::unique_ptr<BRDFIntegrationLUT>      m_BRDFIntegrationLUT;
    std::unique_ptr<PrefilterEnvironment>    m_PrefilterEnvironment;
    std::unique_ptr<IrradianceConvolution>   m_IrradianceConvolution;
    // --------------------------------------------------
    // World 데이터
    // --------------------------------------------------
    std::unique_ptr<Sponza>                  m_Sponza;
}; // Renderer
