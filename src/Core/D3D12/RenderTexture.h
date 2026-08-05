#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class DescriptorHeapAllocator;

class RenderTexture {
public:
    enum class RenderTextureType {
        Normal, // RTV + SRV
        UAV,    // RTV + SRV + UAV
        Depth   // DSV + SRV
    }; // RenderTextureType

    struct InitParams {
        ID3D12Device*            device;
        DescriptorHeapAllocator* rtvAllocator;
        DescriptorHeapAllocator* dsvAllocator;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        UINT                     width;
        UINT                     height;
        DXGI_FORMAT              format;
        RenderTextureType        type;

        InitParams()
            : device(nullptr), rtvAllocator(nullptr), dsvAllocator(nullptr), sharedDescriptorAllocator(nullptr),
            width(0), height(0), format(DXGI_FORMAT_R16G16B16A16_FLOAT), type(RenderTextureType::Normal) {
        }
    }; // InitDefaultParams

public:
    RenderTexture();
    RenderTexture(const RenderTexture&) = delete;
    ~RenderTexture();

    bool Init(const InitParams&);
    void Transition(ID3D12GraphicsCommandList*, D3D12_RESOURCE_STATES);
    void Clear(ID3D12GraphicsCommandList*, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void ClearDepth(ID3D12GraphicsCommandList*, float depth = 1.0f, UINT8 stencil = 0);

public:
    ID3D12Resource*             GetResource() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;
    UINT                        GetSRVIndex() const;
    UINT                        GetUAVIndex() const;
    UINT                        GetWidth() const;
    UINT                        GetHeight() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE            m_rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE            m_dsvHandle;
    UINT                                   m_srvIndex;
    UINT                                   m_uavIndex;
    D3D12_RESOURCE_STATES                  m_currentState;
    RenderTextureType                      m_type;
    UINT                                   m_width;
    UINT                                   m_height;
}; // RenderTexture