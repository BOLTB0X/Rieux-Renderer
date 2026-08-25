#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <memory>

class RenderTexture;
class RenderTextureManager;
class DescriptorHeapAllocator;

class EnvironmentProbe {
public:
    static const UINT FACE_COUNT = 6;

    struct InitParams {
        ID3D12Device*            device;
        RenderTextureManager*    renderTextureManager;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        UINT                     faceSize;   // 256 권장 (캡처 해상도, IBL은 저해상도로 충분)
        DXGI_FORMAT              format;     // DXGI_FORMAT_R16G16B16A16_FLOAT (HDR 필수)

        InitParams()
            : device(nullptr), renderTextureManager(nullptr), sharedDescriptorAllocator(nullptr),
            faceSize(256), format(DXGI_FORMAT_R16G16B16A16_FLOAT) {
        }
    }; // InitParams

public:
    EnvironmentProbe();
    EnvironmentProbe(const EnvironmentProbe&) = delete;
    EnvironmentProbe& operator=(const EnvironmentProbe&) = delete;
    ~EnvironmentProbe();

    bool Init(const InitParams&);
    void SetPosition(const DirectX::XMFLOAT3&);
    void MarkDirty();
    bool IsDirty() const;
    void UpdateFaceMatrices(); // 더티일 때만 호출 - 6개 view 행렬 + 각 면 FrameCB 갱신
    void ClearDirty();

public:
    RenderTexture*              GetCubemapTexture() const;
    RenderTexture*              GetDepthTexture() const;
    D3D12_GPU_VIRTUAL_ADDRESS   GetFaceFrameCBGPUAddress(UINT face) const;
    UINT                        GetFaceSize() const;

private:
    RenderTexture*                                                 m_cubemapTexture;
    RenderTexture*                                                 m_depthTexture;
    std::array<DirectX::XMMATRIX, FACE_COUNT>                      m_faceViewMatrices;
    DirectX::XMMATRIX                                              m_faceProjMatrix;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FACE_COUNT> m_faceFrameCB;
    std::array<UINT8*, FACE_COUNT>                                 m_mappedFaceFrameCB;
    DirectX::XMFLOAT3                                              m_position;
    UINT                                                           m_faceSize;
    bool                                                           m_isDirty;
}; // EnvironmentProbe