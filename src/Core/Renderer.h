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
//class Frustum;
class FrustumCuller;
class DirectionalLight;
class GPUMonitor;
class TextureManager;
class DescriptorHeapAllocator;
class ImGuiManager;
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
        float       rotationDeltaX;
        float       rotationDeltaY;
        float       zoomDelta;

        FrameParams()
            : fps(0), playTime(0.0f), cpuPercentage(0.0f), cpuName(""),
            deltaTime(0.0f), moveForward(0.0f), moveRight(0.0f), moveUp(0.0f),
            rotationDeltaX(0.0f), rotationDeltaY(0.0f), zoomDelta(0.0f) {
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
    std::unique_ptr<RenderQueue>             m_RenderQueue;
    std::unique_ptr<Camera>                  m_Camera;
    //std::unique_ptr<Frustum>                 m_Frustum;
    std::unique_ptr<FrustumCuller>           m_FrustumCuller;
    std::unique_ptr<DirectionalLight>        m_DirectionalLight;
    std::unique_ptr<GPUMonitor>              m_GPUMonitor;
    std::shared_ptr<TextureManager>          m_TextureManager;
    std::shared_ptr<ImGuiManager>            m_ImGuiManager;
    FrameParams                              m_currentFrameParams;
    // --------------------------------------------------
    // World 데이터
    // --------------------------------------------------
    std::unique_ptr<Sponza>                  m_Sponza;
}; // Renderer