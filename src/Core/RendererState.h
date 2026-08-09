#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include <DirectXMath.h>
// Utils
#include "GPUCommons.h"

class RendererState {
// Global 상수
public: // Screen
    static bool  FullScrren;
    static bool  VsyncEnable;
    static int   ScreenWidth;
    static int   ScreenHeight;
    static float ScreenDepth;
    static float ScreenNear;
    static float aspectRatio;

    static UINT FrameCount;
    static UINT CBV_Table;
    static UINT FrameCBIndex;
    static UINT LightCBIndex;
    static UINT WorldIndex;
    static UINT Tex0Index;
    static UINT Tex1Index;
    static UINT Tex2Index;
    static UINT StaticSamplerIndex;

    static UINT MeshDataIndex;
    static UINT MaterialIndicesIndex;
    static UINT VertexBufferIndexParam;
    static UINT InstanceIndexParam;
    static UINT InstanceDataIndex;
    static UINT BindlessTexIndex;
    static UINT BindlessBufIndex;

    static UINT CullingFrustumPlanesIndex;
    static UINT CullingMasterInstanceIndex;
    static UINT CullingMasterCommandsIndex;
    static UINT CullingVisibleMainCommandsIndex;
    static UINT CullingVisibleVaseCommandsIndex;
    static UINT CullingMainCountIndex;
    static UINT CullingVaseCountIndex;

    static UINT HZBConstantsIndex;
    static UINT DepthTextureIndex;
    static UINT HZBTextureIndex;

    static UINT OcclusionConstantsIndex;
    static UINT OcclusionHiZTextureIndex;
    static UINT OcclusionFrustumMainCommandsIndex;
    static UINT OcclusionFrustumVaseCommandsIndex;
    static UINT OcclusionFrustumMainCountIndex;
    static UINT OcclusionFrustumVaseCountIndex;
    static UINT OcclusionMeshInstanceDataIndex;
    static UINT OcclusionFinalMainCommandsIndex;
    static UINT OcclusionFinalVaseCommandsIndex;
    static UINT OcclusionFinalMainCountIndex;
    static UINT OcclusionFinalVaseCountIndex;

    static UINT DebugCameraClipIndex;
    static UINT DebugDepthTexIndex;

    static UINT SharedHeapCapacity;
    static UINT RTVCapacity;
    static UINT DSVCapacity;

    static DXGI_FORMAT RTVFormat;

 // Global CBs
public:
    struct FrameParams {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
        DirectX::XMMATRIX viewInv;
        DirectX::XMMATRIX projInv;
        DirectX::XMFLOAT3 cameraPosition;
        float             cameraFov;
        DirectX::XMFLOAT2 screenResolution;
        float             time;
        DirectX::XMFLOAT3 direction;
        DirectX::XMFLOAT4 ambient;
        DirectX::XMFLOAT4 diffuse;
        DirectX::XMFLOAT3 lookAt;
        DirectX::XMMATRIX lightViewMatrix;
        DirectX::XMMATRIX lightProjectionMatrix;
        float             shadowMapWidth;
        float             shadowMapHeight;
        float             shadowBias;
        float             shadowSpread;

        FrameParams();
    }; // FrameParams

public:
    RendererState();
    RendererState(const RendererState&) = delete;
    RendererState& operator=(const RendererState&) = delete;
    ~RendererState();

    bool Init(ID3D12Device*);
    void Frame(const FrameParams&);
    void Shutdown();

    D3D12_GPU_VIRTUAL_ADDRESS GetFrameCBGPUVirtualAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetLightCBGPUVirtualAddress() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_frameCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_lightCB;
    UINT8*                                   m_mappedFrameCB;
    UINT8*                                   m_mappedLightCB;
}; // RendererState