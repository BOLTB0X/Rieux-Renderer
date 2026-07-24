#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

class D3D12SwapChain {
public:
    struct InitParams {
        HWND                hwnd;
        ID3D12Device*       device;
        ID3D12CommandQueue* commandQueue;
        UINT                width;
        UINT                height;
        UINT                frameCount;

        InitParams() : hwnd(nullptr), device(nullptr), commandQueue(nullptr), width(0), height(0), frameCount(2) {
        }
    }; // InitParams

public:
    D3D12SwapChain();
    D3D12SwapChain(const D3D12SwapChain&) = delete;
    ~D3D12SwapChain();

    bool Init(const InitParams&);
    void Present(bool);

public:
    const D3D12_VIEWPORT&       GetViewport() const;
    const D3D12_RECT&           GetScissorRect() const;
    ID3D12Resource*             GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;
    UINT                        GetCurrentFrameIndex() const;

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>            m_rtvHandles;
    D3D12_VIEWPORT                                      m_viewport;
    D3D12_RECT                                          m_scissorRect;
    UINT                                                m_rtvDescriptorSize;
    UINT                                                m_frameIndex;
    UINT                                                m_frameCount;
}; // D3D12SwapChain