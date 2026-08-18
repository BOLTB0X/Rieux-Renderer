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
