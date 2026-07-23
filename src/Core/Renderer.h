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
#include "D3D12/RenderTarget.h"

class RenderQueue;
class Camera;
class DirectionalLight;
class GPUMonitor;
class TextureManager;
class DescriptorHeapAllocator;
class ImGuiManager;
class D3D12Device;
class CommandQueue;
class D3D12SwapChain;
class RenderTextureManager;

class Renderer {
public:
    struct InitParams {
        HWND                          hwnd;
        int                           width;
        int                           height;
        std::shared_ptr<ImGuiManager> imGuiManager;

        InitParams() : hwnd(nullptr), width(0), height(0), imGuiManager(nullptr) {
        }
    }; // InitParams

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
    bool LoadAssets(HWND);
    bool LoadGUIs(HWND, std::shared_ptr<ImGuiManager>);

    bool InitSharedDescriptorHeap();
    bool InitSceneRenderTarget(int width, int height);
    bool InitCommonCBs();
    void UpdateCommonCBs();
    void ShutdownCommonCBs();

    void PopulateCommandList();

    void OnGUI();

private:
    // --------------------------------------------------
    // 파이프라인 객체
    // --------------------------------------------------
    std::unique_ptr<D3D12Device>          m_D3D12Device;
    std::unique_ptr<CommandQueue>         m_CommandQueue; // 메인 다이렉트 큐
    std::unique_ptr<D3D12SwapChain>       m_SwapChain;    // 백버퍼 + Present 전용
    D3D12_VIEWPORT                        m_viewport;
    D3D12_RECT                            m_scissorRect;
    DXGI_FORMAT                           m_rtvFormat;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    std::unique_ptr<DescriptorHeapAllocator>    m_sharedDescriptorAllocator;
    std::unique_ptr<RenderTextureManager>       m_RenderTextureManager;
    RenderTarget                                m_SceneRenderTarget;

    // --------------------------------------------------
    // 공용 데이터
    // --------------------------------------------------
    std::unique_ptr<RenderQueue>           m_RenderQueue;
    std::unique_ptr<Camera>                m_Camera;
    std::unique_ptr<DirectionalLight>      m_DirectionalLight;
    std::unique_ptr<GPUMonitor>            m_GPUMonitor;
    std::shared_ptr<TextureManager>        m_TextureManager;
    std::shared_ptr<ImGuiManager>          m_ImGuiManager;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_frameCB;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_lightCB;

    UINT8* m_mappedFrameCB;
    UINT8* m_mappedLightCB;

    FrameParams                            m_currentFrameParams;
}; // Renderer