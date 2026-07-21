#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <string>

class Camera;
class DirectionalLight;
class GPUMonitor;
class ImGuiManager;

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
    bool LoadAssets();
    bool LoadGUIs(HWND, std::shared_ptr<ImGuiManager>);

    bool InitCommonCBs();
    void UpdateCommonCBs();
	void ShutdownCommonCBs();


    void PopulateCommandList();
    void WaitForPreviousFrame();

    void OnGUI();

private:
    static const UINT FrameCount = 2; // 더블 버퍼링

    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    // --------------------------------------------------
    // 파이프라인 객체
    // --------------------------------------------------
    D3D12_VIEWPORT                                      m_viewport;
    D3D12_RECT                                          m_scissorRect;
    DXGI_FORMAT                                         m_rtvFormat;
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    Microsoft::WRL::ComPtr<IDXGIAdapter3>               m_iDXGIAdapter3;
    UINT                                                m_rtvDescriptorSize;

    // --------------------------------------------------
    // 에셋 리소스
    // --------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                            m_vertexBufferView;

    // --------------------------------------------------
    // 공용 데이터
    // --------------------------------------------------
    std::unique_ptr<Camera>                             m_Camera;
    std::unique_ptr<DirectionalLight>                   m_DirectionalLight;
    std::unique_ptr<GPUMonitor>                         m_GPUMonitor;
    std::shared_ptr<ImGuiManager>                       m_ImGuiManager;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_frameCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_lightCB;

    // Persistent Mapping을 위한 포인터
    UINT8*                                              m_mappedFrameCB;
    UINT8*                                              m_mappedLightCB;

    // --------------------------------------------------
    // 동기화 객체
    // --------------------------------------------------
    UINT                                                m_frameIndex;
    HANDLE                                              m_fenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValue;

    FrameParams                                         m_currentFrameParams;
}; // Renderer