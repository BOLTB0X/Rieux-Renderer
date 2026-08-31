#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>
// Utils
#include "GPUCommons.h"

class RendererDebugger;

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
    static UINT ShadowMapIndex;

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

    static UINT OcclusionPhaseIndex;
    static UINT OcclusionCulledMainCommandsIndex;
    static UINT OcclusionCulledVaseCommandsIndex;
    static UINT OcclusionCulledMainCountIndex;
    static UINT OcclusionCulledVaseCountIndex;

    static UINT ShadowCullingDataIndex;
    static UINT ShadowCullingMasterInstanceIndex;
    static UINT ShadowCullingMasterCommandsIndex;
    static UINT ShadowCullingVisibleCommandsCascade0Index;
    static UINT ShadowCullingVisibleCommandsCascade1Index;
    static UINT ShadowCullingVisibleCommandsCascade2Index;
    static UINT ShadowCullingVisibleCommandsCascade3Index;
    static UINT ShadowCullingDrawCountsIndex;

    static UINT ShadowCullingMainVisibleCommandsCascade0Index;
    static UINT ShadowCullingMainVisibleCommandsCascade1Index;
    static UINT ShadowCullingMainVisibleCommandsCascade2Index;
    static UINT ShadowCullingMainVisibleCommandsCascade3Index;

    static UINT ShadowCullingVaseVisibleCommandsCascade0Index;
    static UINT ShadowCullingVaseVisibleCommandsCascade1Index;
    static UINT ShadowCullingVaseVisibleCommandsCascade2Index;
    static UINT ShadowCullingVaseVisibleCommandsCascade3Index;

    static UINT ShadowCullingMainDrawCountsIndex;
    static UINT ShadowCullingVaseDrawCountsIndex;

    static UINT CascadeIndexConstant;
    static UINT CSMShadowMapIndex;

    static UINT BRDFLUTConstantBufferIndex;
    static UINT BRDFLUTIndex;

    static UINT PrefilterConstantBufferIndex;
    static UINT SourceCubemapIndex;
    static UINT OutputMipFaceIndex;

    static UINT IrradianceConstantIndex;
    static UINT IrradianceSourceCubemapIndex;
    static UINT IrradianceOutputIndex;

    static UINT DeferredFrameCBIndex;
    static UINT DeferredLightCBIndex;
    static UINT DeferredGBuffer0Index;
    static UINT DeferredGBuffer1Index;
    static UINT DeferredDepthIndex;
    static UINT DeferredCSMIndex;
    static UINT DeferredIrradianceIndex;
    static UINT DeferredPrefilterIndex;
    static UINT DeferredBRDFLUTIndex;

    static UINT ToneMappingHDRTexIndex;

    static UINT SSRFrameCBIndex;
    static UINT SSRLitSceneIndex;
    static UINT SSRDepthIndex;
    static UINT SSRGBuffer1Index;

    static UINT DebugCameraClipIndex;
    static UINT DebugDepthTexIndex;

    static UINT DebugFrameIndex;
    static UINT DebugInstanceIndex;
    static UINT DebugInstanceDataIndex;
    static UINT DebugLineFrameIndex;

    static UINT SharedHeapCapacity;
    static UINT RTVCapacity;
    static UINT DSVCapacity;

    static DXGI_FORMAT RTVFormat;
    static DXGI_FORMAT ImGUIRTVFormat;

 // Global CBs
public:
    struct RuntimeFrameParams {
        int         fps;
        float       playTime;
        float       cpuPercentage;
        std::string cpuName;
        float       deltaTime;
        float       moveForward;
        float       moveRight;
        float       moveUp;
        float       masterMoveForward;
        float       masterMoveRight;
        float       masterMoveUp;
        float       rotationDeltaX;
        float       rotationDeltaY;
        float       zoomDelta;
        float       sceneRotationDeltaX;
        float       sceneRotationDeltaY;
        bool        toggleCameraMode;

        RuntimeFrameParams()
            : fps(0), playTime(0.0f), cpuPercentage(0.0f), cpuName(""),
            deltaTime(0.0f), moveForward(0.0f), moveRight(0.0f), moveUp(0.0f),
            masterMoveForward(0.0f), masterMoveRight(0.0f), masterMoveUp(0.0f),
            rotationDeltaX(0.0f), rotationDeltaY(0.0f), zoomDelta(0.0f),
            sceneRotationDeltaX(0.0f), sceneRotationDeltaY(0.0f),
            toggleCameraMode(false) {
        }
    }; // RuntimeFrameParams

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

        DirectX::XMMATRIX cascadeViewProj[GPUCommons::MAX_CASCADES];

        DirectX::XMFLOAT4 cascadeSplits;

        UINT              cascadeCount;

        FrameParams();
    }; // FrameParams

public:
    RendererState();
    RendererState(const RendererState&) = delete;
    RendererState& operator=(const RendererState&) = delete;
    ~RendererState();

    bool Init(ID3D12Device*);
    void Frame(const FrameParams&);
    void FrameScene(const FrameParams&);
    void Shutdown();

public:
    void SetCurrentFrameParams(const RuntimeFrameParams&);
    void SetUseMasterCamera(bool);
    bool IsUsingMasterCamera() const;

    const RuntimeFrameParams& GetCurrentFrameParams() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetFrameCBGPUVirtualAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetSceneFrameCBGPUVirtualAddress() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetLightCBGPUVirtualAddress() const;

private:
    struct DebugLineVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;

        DebugLineVertex() : position(0.0f, 0.0f, 0.0f), color(0.0f, 0.0f, 0.0f) {
        }

        DebugLineVertex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& col)
            : position(pos), color(col) {
        }
    }; // DebugLineVertex

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_frameCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_sceneFrameCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_lightCB;
    UINT8*                                   m_mappedFrameCB;
    UINT8*                                   m_mappedSceneFrameCB;
    UINT8*                                   m_mappedLightCB;
    RuntimeFrameParams                       m_currentFrameParams;
    bool                                     m_useMasterCamera;
    Microsoft::WRL::ComPtr<ID3D12Resource>   m_sceneCameraFrustumBuffer;
    D3D12_VERTEX_BUFFER_VIEW                 m_sceneCameraFrustumVBV;
    DebugLineVertex*                         m_sceneCameraFrustumMappedData;

    friend class RendererDebugger;
}; // RendererState
