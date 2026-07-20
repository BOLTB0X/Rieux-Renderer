#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>        // ComPtr
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <string>


class Renderer {
public:
    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer();

    bool Init(HWND hwnd, int width, int height);
    void Shutdown();
    bool Frame();
    bool Render();

private:
    // DX12 초기화 과정을 두 단계로 분리
    bool LoadPipeline(HWND hwnd, int width, int height);
    bool LoadAssets();

    void PopulateCommandList();
    void WaitForPreviousFrame();

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
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    UINT                                                m_rtvDescriptorSize;

    // --------------------------------------------------
    // 에셋 리소스
    // --------------------------------------------------
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                            m_vertexBufferView;

    // --------------------------------------------------
    // 동기화 객체
    // --------------------------------------------------
    UINT                                                m_frameIndex;
    HANDLE                                              m_fenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValue;
};