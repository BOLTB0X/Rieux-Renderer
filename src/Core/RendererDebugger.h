#pragma once
#include <d3d12.h>

class Camera;
class D3D12SwapChain;
class DescriptorHeapAllocator;
class HierarchicalZBuffer;
class OcclusionCuller;
class PSOManager;
class RenderTexture;
class RenderTextureManager;
class RendererState;
class Sponza;

class RendererDebugger {
public:
    struct InitParams {
        ID3D12Device*              device;
        RendererState*             rendererState;
        RenderTextureManager*      renderTextureManager;
        D3D12SwapChain*            swapChain;
        PSOManager*                psoManager;
        DescriptorHeapAllocator*   sharedDescriptorAllocator;
        Camera*                    sceneCamera;
        Camera*                    masterCamera;
        OcclusionCuller*           occlusionCuller;
        Sponza*                    sponza;

        InitParams() : device(nullptr), rendererState(nullptr),
            renderTextureManager(nullptr), swapChain(nullptr), psoManager(nullptr),
            sharedDescriptorAllocator(nullptr), sceneCamera(nullptr), masterCamera(nullptr),
            occlusionCuller(nullptr), sponza(nullptr) {
        }
    }; // InitParams

public:
    RendererDebugger();
    RendererDebugger(const RendererDebugger&) = delete;
    RendererDebugger& operator=(const RendererDebugger&) = delete;
    ~RendererDebugger();

    bool Init(const InitParams&);
    void Frame();
    void Shutdown();

    void DebugDepthRenderTextures(ID3D12GraphicsCommandList*, RenderTexture*);
    void DebugHiZRenderTextures(ID3D12GraphicsCommandList*);
    void DebugBoundingBox(ID3D12GraphicsCommandList*);
    void DebugSceneCameraFrustum(ID3D12GraphicsCommandList*);

    Camera*       GetActiveCamera();
    const Camera* GetActiveCamera() const;

private:
    bool BuildSceneCameraFrustumBuffer();

private:
    ID3D12Device*              m_device;
    RendererState*             m_rendererState;
    RenderTextureManager*      m_renderTextureManager;
    D3D12SwapChain*            m_swapChain;
    PSOManager*                m_psoManager;
    DescriptorHeapAllocator*   m_sharedDescriptorAllocator;
    Camera*                    m_sceneCamera;
    Camera*                    m_masterCamera;
    OcclusionCuller*           m_occlusionCuller;
    Sponza*                    m_sponza;
}; // RendererDebugger
