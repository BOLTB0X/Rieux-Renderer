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
class RendererState;

class Renderer {
public:
    struct InitParams {
        HWND                          hwnd;
        std::shared_ptr<ImGuiManager> imGuiManager;

        InitParams() : hwnd(nullptr), imGuiManager(nullptr) {
        }
    }; // InitDefaultParams

    struct FrameParams {
        int         fps;
        float       playTime;
        float       cpuPercentage;
        std::string cpuName;
        float       deltaTime;
        float       moveForward;
        float       moveRight;
        float       moveUp;
        float       masterMoveForward;
        float       masterMoveRight;
        float       masterMoveUp;
        float       rotationDeltaX;
        float       rotationDeltaY;
        float       zoomDelta;
        float       sceneRotationDeltaX;
        float       sceneRotationDeltaY;
        bool        toggleCameraMode;

        FrameParams()
            : fps(0), playTime(0.0f), cpuPercentage(0.0f), cpuName(""),
            deltaTime(0.0f), moveForward(0.0f), moveRight(0.0f), moveUp(0.0f),
            masterMoveForward(0.0f), masterMoveRight(0.0f), masterMoveUp(0.0f),
            rotationDeltaX(0.0f), rotationDeltaY(0.0f), zoomDelta(0.0f),
            sceneRotationDeltaX(0.0f), sceneRotationDeltaY(0.0f),
            toggleCameraMode(false) {
        }
    }; // FrameParams

public:
    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer();

    bool Init(const InitParams&);
    bool Frame(const FrameParams&);
    void Shutdown();

private:
    bool Render();
    bool LoadPipeline(HWND, int, int);
    bool LoadSceneRenderTarget(int, int);
    bool LoadAssets(HWND);
    bool LoadGUIs(HWND, std::shared_ptr<ImGuiManager>);

private:
    void PopulateCommandList();

    void FrustumPass(ID3D12GraphicsCommandList*);
    void DepthPass(ID3D12GraphicsCommandList*);
    void ShadowPass(ID3D12GraphicsCommandList*);
    void OcclusionPhase1Pass(ID3D12GraphicsCommandList*);
    void OcclusionPhase2Pass(ID3D12GraphicsCommandList*);
    void SponzaPass(ID3D12GraphicsCommandList*);

    void OnGUI();

private:
    void DebugDepthRenderTextures(ID3D12GraphicsCommandList*, RenderTexture*);
    void DebugHiZRenderTextures(ID3D12GraphicsCommandList*);
    void DebugBoundingBox(ID3D12GraphicsCommandList*);
    void DebugSceneCameraFrustum(ID3D12GraphicsCommandList*);

    Camera*       GetActiveCamera();
    const Camera* GetActiveCamera() const;
    bool          BuildSceneCameraFrustumBuffer();
    void          UpdateSceneCameraFrustum();

private:
    struct DebugLineVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

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
    std::unique_ptr<RenderQueue>             m_RenderQueue;
    std::unique_ptr<Camera>                  m_SceneCamera;
    std::unique_ptr<Camera>                  m_MasterCamera;
    std::unique_ptr<FrustumCuller>           m_FrustumCuller;
    std::unique_ptr<HierarchicalZBuffer>     m_HierarchicalZBuffer;
    std::unique_ptr<OcclusionCuller>         m_OcclusionCuller;
    std::unique_ptr<DirectionalLight>        m_DirectionalLight;
    std::unique_ptr<GPUMonitor>              m_GPUMonitor;
    std::shared_ptr<TextureManager>          m_TextureManager;
    std::shared_ptr<ImGuiManager>            m_ImGuiManager;
    FrameParams                              m_currentFrameParams;
    bool                                     m_useMasterCamera;
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_sceneCameraFrustumBuffer;
    D3D12_VERTEX_BUFFER_VIEW                  m_sceneCameraFrustumVBV;
    DebugLineVertex*                          m_sceneCameraFrustumMappedData;
    // --------------------------------------------------
    // World 데이터
    // --------------------------------------------------
    std::unique_ptr<Sponza>                  m_Sponza;
}; // Renderer
