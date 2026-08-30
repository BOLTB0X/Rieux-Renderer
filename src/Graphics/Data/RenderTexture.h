#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

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
        UINT                     mipLevels;
        DXGI_FORMAT              format;
        RenderTextureType        type;
        float                    depthClearValue;
        UINT                     arraySize;
        bool                     isCubeMap;

        InitParams()
            : device(nullptr), rtvAllocator(nullptr), dsvAllocator(nullptr), sharedDescriptorAllocator(nullptr),
            width(0), height(0), mipLevels(1), format(DXGI_FORMAT_R16G16B16A16_FLOAT),
            type(RenderTextureType::Normal), depthClearValue(0.0f), arraySize(1), isCubeMap(false) {
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
    void                        SetCurrentStateWithoutBarrier(D3D12_RESOURCE_STATES);

    RenderTextureType           GetType() const;
    ID3D12Resource*             GetResource() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;
    UINT                        GetSRVIndex() const;
    UINT                        GetSliceSRVIndex(UINT) const;
    UINT                        GetUAVIndex() const;
    UINT                        GetWidth() const;
    UINT                        GetHeight() const;
    UINT                        GetMipLevels() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetMipSRVGPUHandle(UINT) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetMipUAVGPUHandle(UINT) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(UINT) const;
    UINT                        GetMipFaceUAVIndex(UINT, UINT) const;
    UINT                        GetMipFaceSRVIndex(UINT, UINT) const;
    UINT                        GetArraySize() const;

    bool                        IsCubeMap() const;

private:
    bool CreateTextureResource(const InitParams&);
    bool CreateDepthViews(const InitParams&);
    bool CreateRenderTargetViews(const InitParams&);
    bool CreateShaderResourceViews(const InitParams&);
    bool CreateUnorderedAccessViews(const InitParams&);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE              m_rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE              m_dsvHandle;
    UINT                                     m_srvIndex;
    UINT                                     m_uavIndex;
    D3D12_RESOURCE_STATES                    m_currentState;
    RenderTextureType                        m_type;
    UINT                                     m_width;
    UINT                                     m_height;
    std::vector<UINT>                        m_mipSrvIndices;
    std::vector<UINT>                        m_mipUavIndices;
    std::vector<UINT>                        m_sliceSrvIndices;
    bool                                     m_isCubeMap;
    UINT                                     m_mipLevels;
    DescriptorHeapAllocator*                 m_sharedDescriptorAllocator;

    UINT                                     m_arraySize;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_dsvSliceHandles;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvSliceHandles;
    std::vector<UINT>                        m_mipFaceUavIndices;
    std::vector<UINT>                        m_mipFaceSrvIndices;
}; // RenderTexture
