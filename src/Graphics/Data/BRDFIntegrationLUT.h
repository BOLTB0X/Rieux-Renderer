#pragma once
#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class RenderTexture;
class RenderTextureManager;
class DescriptorHeapAllocator;

class BRDFIntegrationLUT {
public:
    struct InitParams {
        ID3D12Device*            device;
        ID3D12RootSignature*     rootSignature;
        ID3D12PipelineState*     computePSO;
        RenderTextureManager*    renderTextureManager;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        DXGI_FORMAT              format;

        InitParams()
            : device(nullptr), rootSignature(nullptr), computePSO(nullptr),
            renderTextureManager(nullptr), sharedDescriptorAllocator(nullptr),
            format(DXGI_FORMAT_R16G16_FLOAT) {
        }
    }; // InitParams

public:
    BRDFIntegrationLUT();
    BRDFIntegrationLUT(const BRDFIntegrationLUT&) = delete;
    BRDFIntegrationLUT& operator=(const BRDFIntegrationLUT&) = delete;
    ~BRDFIntegrationLUT();

    bool Init(const InitParams&);
    void Generate(ID3D12GraphicsCommandList*);

public:
    RenderTexture*            GetTexture() const;
    ID3D12Resource*           GetResource() const;
    UINT                      GetSRVIndex() const;
    UINT                      GetLUTSize() const;
    bool                      IsGenerated() const;

private:
    struct BRDFLUTCB {
        UINT lutSize;
        UINT padding[3];
    };

private:
    ID3D12RootSignature*                                  m_rootSignature;
    ID3D12PipelineState*                                  m_computePSO;
    RenderTexture*                                        m_lutTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>                m_constantBuffer;
    UINT8*                                                m_mappedConstantBuffer;
    DescriptorHeapAllocator*                              m_sharedDescriptorAllocator;
    UINT                                                  m_lutSize;
    DXGI_FORMAT                                           m_format;
    bool                                                  m_isGenerated;
}; // BRDFIntegrationLUT
