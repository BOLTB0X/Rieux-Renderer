#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class RenderTexture;
class RenderTextureManager;
class DescriptorHeapAllocator;

class IrradianceConvolution {
public:
    struct InitParams {
        ID3D12Device*            device;
        ID3D12RootSignature*     rootSignature;
        ID3D12PipelineState*     computePSO;
        RenderTextureManager*    renderTextureManager;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        RenderTexture*           sourceCubemap;
        UINT                     outputSize;  // 32
        float                    sampleDelta; // 0.025

        InitParams()
            : device(nullptr), rootSignature(nullptr), computePSO(nullptr),
            renderTextureManager(nullptr), sharedDescriptorAllocator(nullptr),
            sourceCubemap(nullptr), outputSize(32), sampleDelta(0.025f) {
        }
    }; // InitParams

public:
    IrradianceConvolution();
    IrradianceConvolution(const IrradianceConvolution&) = delete;
    IrradianceConvolution& operator=(const IrradianceConvolution&) = delete;
    ~IrradianceConvolution();

    bool Init(const InitParams&);
    void Generate(ID3D12GraphicsCommandList*);
    void Invalidate();

public:
    RenderTexture*  GetTexture() const;
    UINT            GetSRVIndex() const;
    bool            IsGenerated() const;

private:
    struct IrradianceRootConstants {
        UINT  faceIndex;
        UINT  outputSize;
        float sampleDelta;
        float padding;
    }; // IrradianceRootConstants

private:
    ID3D12RootSignature*     m_rootSignature;
    ID3D12PipelineState*     m_computePSO;
    RenderTexture*           m_sourceCubemap;
    RenderTexture*           m_outputTexture;
    DescriptorHeapAllocator* m_sharedDescriptorAllocator;
    UINT                     m_outputSize;
    float                    m_sampleDelta;
    bool                     m_isGenerated;
}; // IrradianceConvolution