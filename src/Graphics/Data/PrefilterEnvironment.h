#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class RenderTexture;
class RenderTextureManager;
class DescriptorHeapAllocator;

class PrefilterEnvironment {
public:
    struct InitParams {
        ID3D12Device*            device;
        ID3D12RootSignature*     rootSignature;
        ID3D12PipelineState*     computePSO;
        RenderTextureManager*    renderTextureManager;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        RenderTexture*           sourceCubemap;
        UINT                     outputSize;
        UINT                     mipLevels;
        UINT                     sampleCount;
        DXGI_FORMAT              format;

        InitParams()
            : device(nullptr), rootSignature(nullptr), computePSO(nullptr),
            renderTextureManager(nullptr), sharedDescriptorAllocator(nullptr),
            sourceCubemap(nullptr), outputSize(256), mipLevels(5),
            sampleCount(1024), format(DXGI_FORMAT_R16G16B16A16_FLOAT) {
        }
    }; // InitParams

public:
    PrefilterEnvironment();
    PrefilterEnvironment(const PrefilterEnvironment&) = delete;
    PrefilterEnvironment& operator=(const PrefilterEnvironment&) = delete;
    ~PrefilterEnvironment();

    bool Init(const InitParams&);
    void Generate(ID3D12GraphicsCommandList*);
    void Invalidate();

public:
    RenderTexture*  GetTexture() const;
    ID3D12Resource* GetResource() const;
    UINT            GetSRVIndex() const;
    UINT            GetOutputSize() const;
    UINT            GetMipLevels() const;
    bool            IsGenerated() const;

private:
    struct PrefilterCB {
        float roughness;
        UINT  faceIndex;
        UINT  outputSize;
        UINT  sampleCount;

        PrefilterCB() : roughness(0.0f), faceIndex(0), outputSize(0), sampleCount(0) {
        }
    }; // PrefilterCB

private:
    ID3D12RootSignature*                   m_rootSignature;
    ID3D12PipelineState*                   m_computePSO;
    RenderTexture*                         m_sourceCubemap;
    RenderTexture*                         m_outputTexture;
    DescriptorHeapAllocator*               m_sharedDescriptorAllocator;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    UINT8*                                 m_mappedConstantBuffer;
    UINT                                   m_outputSize;
    UINT                                   m_mipLevels;
    UINT                                   m_sampleCount;
    DXGI_FORMAT                            m_format;
    bool                                   m_isGenerated;
}; // PrefilterEnvironment
