#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class DescriptorHeapAllocator;

class RenderTarget {
public:
    struct InitParams {
        ID3D12Device*            device;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        UINT                     width;
        UINT                     height;
        DXGI_FORMAT              colorFormat;
        DXGI_FORMAT              depthFormat;

        InitParams()
            : device(nullptr), sharedDescriptorAllocator(nullptr), width(0), height(0),
            colorFormat(DXGI_FORMAT_R8G8B8A8_UNORM), depthFormat(DXGI_FORMAT_D32_FLOAT) {
        }
    }; // InitDefaultParams

public:
    RenderTarget();
    RenderTarget(const RenderTarget&) = delete;
    ~RenderTarget();

    bool Init(const InitParams&);
    void BeginRender(ID3D12GraphicsCommandList*, const float clearColor[4]);
    void EndRender(ID3D12GraphicsCommandList*);

    UINT                  GetColorSRVIndex() const;
    ID3D12Resource*       GetColorResource() const;
    const D3D12_VIEWPORT& GetViewport() const;
    const D3D12_RECT&     GetScissorRect() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_colorTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE                  m_rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE                  m_dsvHandle;
    UINT                                         m_colorSRVIndex;
    D3D12_VIEWPORT                               m_viewport;
    D3D12_RECT                                   m_scissorRect;
}; // RenderTarget