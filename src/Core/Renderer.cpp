#include "Pch.h"
#include "Renderer.h"
#include "RendererDebugger.h"
#include "RendererState.h"
// D3D12
#include "D3D12Device.h"
#include "CommandQueue.h"
#include "D3D12SwapChain.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "D3D12/RenderTarget.h"
// Managers
#include "RenderTextureManager.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "ImGuiManager.h"
// Graphics/Tools
#include "DescriptorHeapAllocator.h"
// Components
#include "RenderQueue.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "GPUMonitor.h"
// Techniques
#include "FrustumCuller.h"
#include "HierarchicalZBuffer.h"
#include "OcclusionCuller.h"
#include "CascadedShadowMap.h"
// World
#include "Sponza.h"
// Utils
#include "DebugHelper.h"
#include "SharedCommons.h"
#include "GPUCommons.h"
#include "FunctionWidget.h"
// PIX
#include <pix3.h>

using namespace DebugHelper;
using Microsoft::WRL::ComPtr;

Renderer::Renderer() {
    m_D3D12Device = std::make_unique<D3D12Device>();
    m_CommandQueue = std::make_unique<CommandQueue>();
    m_SwapChain = std::make_unique<D3D12SwapChain>();
    m_SceneRenderTarget = std::make_unique<RenderTarget>();
    m_RendererState = std::make_unique<RendererState>();
    m_RendererDebugger = std::make_unique<RendererDebugger>();

    // 컴포넌트 및 매니저 객체 생성
    m_RenderQueue = std::make_unique<RenderQueue>();
    m_SceneCamera = std::make_unique<Camera>();
    m_MasterCamera = std::make_unique<Camera>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
	m_HierarchicalZBuffer = std::make_unique<HierarchicalZBuffer>();
    m_OcclusionCuller = std::make_unique<OcclusionCuller>();
    m_CascadedShadowMap = std::make_unique<CascadedShadowMap>();

    m_DirectionalLight = std::make_unique<DirectionalLight>();
    m_GPUMonitor = std::make_unique<GPUMonitor>();
    m_TextureManager = std::make_shared<TextureManager>();
    m_sharedDescriptorAllocator = std::make_unique<DescriptorHeapAllocator>();

    m_RenderTextureManager = std::make_unique<RenderTextureManager>();
    m_PSOManager = std::make_shared<PSOManager>();
    m_Sponza = std::make_unique<Sponza>();
} // Renderer

Renderer::~Renderer() {
    Shutdown();
} // ~Renderer

bool Renderer::Init(const InitParams& params) {
    if (!params.hwnd || !params.imGuiManager) {
        return false;
    }

    if (!LoadPipeline(params.hwnd, RendererState::ScreenWidth, RendererState::ScreenHeight)) {
        return false;
    }

    if (!LoadSceneRenderTarget(RendererState::ScreenWidth, RendererState::ScreenHeight)) {
        return false;
    }

    if (!LoadAssets(params.hwnd)) {
        return false;
    }

    if (!LoadGUIs(params.hwnd, params.imGuiManager)) {
        return false;
    }

    return true;
} // Init

void Renderer::Shutdown() {
    if (m_CommandQueue) {
        m_CommandQueue->WaitForPreviousFrame();
    }

    // UI 리소스 및 렌더 타겟 참조 해제
    m_ImGuiManager.reset();

    // LoadAssets 해제
    m_RendererDebugger->Shutdown();
    m_OcclusionCuller.reset();
    m_HierarchicalZBuffer.reset();
	m_FrustumCuller.reset();
    m_Sponza.reset();
    m_TextureManager.reset();
    m_PSOManager.reset();

    // LoadSceneRenderTarget 해제
    m_SceneRenderTarget.reset();
    m_RenderTextureManager.reset();

    // LoadPipeline  해제
    m_GPUMonitor.reset();

    // 상수 버퍼 해제
    m_RendererState->Shutdown();

    m_sharedDescriptorAllocator.reset();
    m_SwapChain.reset();

    if (m_CommandQueue) {
        m_CommandQueue->Shutdown();
        m_CommandQueue.reset();
    }

    // 기타 컴포넌트 해제
    m_RenderQueue.reset();
    m_DirectionalLight.reset();
    m_MasterCamera.reset();
    m_SceneCamera.reset();

    // 디바이스 최종 해제
    m_D3D12Device.reset();
} // Shutdown

bool Renderer::Frame(const FrameParams& frameParams) {
    m_RendererState->SetCurrentFrameParams(frameParams);
    bool enteredMasterCamera = false;

    if (frameParams.toggleCameraMode) {
        m_RendererState->SetUseMasterCamera(!m_RendererState->IsUsingMasterCamera());

        if (m_RendererState->IsUsingMasterCamera()) {
            enteredMasterCamera = true;
            m_MasterCamera->SetPosition(m_SceneCamera->GetPosition());
            m_MasterCamera->SetRotation(m_SceneCamera->GetRotation());
            m_MasterCamera->SetFov(m_SceneCamera->GetFov());
            m_MasterCamera->SetAspect(m_SceneCamera->GetAspect());
            m_MasterCamera->SetNear(m_SceneCamera->GetNear());
            m_MasterCamera->SetFar(m_SceneCamera->GetFar());
            m_MasterCamera->Update();
        }
    } // if (frameParams.toggleCameraMode)

    if (m_RendererState->IsUsingMasterCamera()) {
        if (!enteredMasterCamera) {
            m_MasterCamera->Frame(
                frameParams.masterMoveForward,
                frameParams.masterMoveRight,
                frameParams.masterMoveUp,
                frameParams.rotationDeltaX,
                frameParams.rotationDeltaY,
                frameParams.zoomDelta);

            m_SceneCamera->Frame(
                frameParams.moveForward,
                frameParams.moveRight,
                frameParams.moveUp,
                frameParams.sceneRotationDeltaX,
                frameParams.sceneRotationDeltaY,
                0.0f);
        }
    }
    else {
        m_SceneCamera->Frame(
            frameParams.moveForward,
            frameParams.moveRight,
            frameParams.moveUp,
            frameParams.rotationDeltaX,
            frameParams.rotationDeltaY,
            frameParams.zoomDelta);
    } // if - else

    m_FrustumCuller->Frame(m_SceneCamera->GetViewMatrix(), m_SceneCamera->GetStandardZProjectionMatrix());
    m_DirectionalLight->Frame();
    m_GPUMonitor->Frame();

    OcclusionCuller::FrameParams occParams;
    occParams.viewMatrix = m_SceneCamera->GetViewMatrix();
    occParams.projectionMatrix = m_SceneCamera->GetReverseZProjectionMatrix();
    occParams.screenWidth = static_cast<float>(RendererState::ScreenWidth);
    occParams.screenHeight = static_cast<float>(RendererState::ScreenHeight);
    m_OcclusionCuller->Frame(occParams);

    Camera* renderCamera = m_RendererDebugger->GetActiveCamera();

    CascadedShadowMap::FrameParams csmFrameParams;
    csmFrameParams.nearZ = renderCamera->GetNear();
    csmFrameParams.farZ = renderCamera->GetFar();
    csmFrameParams.fov = renderCamera->GetFov();
    csmFrameParams.camerViewMatrix = renderCamera->GetViewMatrix();
    csmFrameParams.lightDir = m_DirectionalLight->GetDirection();
    m_CascadedShadowMap->Frame(csmFrameParams);

    RendererState::FrameParams stateParams;
    stateParams.view = renderCamera->GetViewMatrix();
    stateParams.projection = renderCamera->GetReverseZProjectionMatrix();
    stateParams.cameraPosition = renderCamera->GetPosition();
    stateParams.cameraFov = renderCamera->GetFov();
    stateParams.direction = m_DirectionalLight->GetDirection();
    stateParams.ambient = m_DirectionalLight->GetAmbient();
    stateParams.diffuse = m_DirectionalLight->GetDiffuse();
    stateParams.lookAt = m_DirectionalLight->GetLookAt();
    stateParams.lightViewMatrix = m_DirectionalLight->GetViewMatrix();
    stateParams.lightProjectionMatrix = m_DirectionalLight->GetProjection();
    stateParams.shadowMapWidth = SharedCommons::SHADOWMAP_WIDTH;
    stateParams.shadowMapHeight = SharedCommons::SHADOWMAP_HEIGHT;
    stateParams.shadowBias = SharedCommons::SHADOW_BIAS;
    stateParams.shadowSpread = SharedCommons::SHADOW_SPREAD;
    stateParams.cascadeCount = m_CascadedShadowMap->GetCascadeCount();
    for (UINT i = 0; i < stateParams.cascadeCount; ++i) {
        stateParams.cascadeViewProj[i] = m_CascadedShadowMap->GetCascadeViewProj(i);
    }
    stateParams.cascadeSplits = m_CascadedShadowMap->GetSplitDistances();

    m_RendererState->Frame(stateParams);

    RendererState::FrameParams sceneFrameParams = stateParams;
    sceneFrameParams.view = m_SceneCamera->GetViewMatrix();
    sceneFrameParams.projection = m_SceneCamera->GetReverseZProjectionMatrix();
    sceneFrameParams.cameraPosition = m_SceneCamera->GetPosition();
    sceneFrameParams.cameraFov = m_SceneCamera->GetFov();
    m_RendererState->FrameScene(sceneFrameParams);

    //m_RendererDebugger->Frame();
    
    return Render();
} // Frame

bool Renderer::Render() {
    m_ImGuiManager->Render();

    PopulateCommandList();

    m_CommandQueue->Execute();
    m_SwapChain->Present(true);
    m_CommandQueue->WaitForPreviousFrame();

    return true;
} // Render

bool Renderer::LoadPipeline(HWND hwnd, int width, int height) {
    // 디바이스 핵심 래퍼 초기화
    if (!m_D3D12Device->Init()) {
        DebugHelper::DebugPrint("D3D12Device 초기화 실패");
        return false;
    }
    else {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_D3D12Device->GetDevice()->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        }
    }

    // 메인 다이렉트 커맨드 큐 초기화
    if (!m_CommandQueue->Init(m_D3D12Device->GetDevice())) {
        DebugHelper::DebugPrint("CommandQueue 초기화 실패");
        return false;
    }

    // 백버퍼 및 프리젠트 전용 스왑체인 초기화 파라미터 조립
    D3D12SwapChain::InitParams swapChainParams;
    swapChainParams.hwnd = hwnd;
    swapChainParams.device = m_D3D12Device->GetDevice();
    swapChainParams.commandQueue = m_CommandQueue->GetQueue();
    swapChainParams.width = width;
    swapChainParams.height = height;
    swapChainParams.frameCount = RendererState::FrameCount;

    if (!m_SwapChain->Init(swapChainParams)) {
        DebugHelper::DebugPrint("D3D12SwapChain 초기화 실패");
        return false;
    }

    // 공유 디스크립터 힙 초기화
    if (!m_sharedDescriptorAllocator->Init(m_D3D12Device->GetDevice(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        RendererState::SharedHeapCapacity, true)) {
        DebugHelper::DebugPrint("InitSharedDescriptorHeap() 실패");
        return false;
    }

    // GPU 프로파일러 초기화
    GPUMonitor::InitParams gpuParams;
    gpuParams.adapter = m_D3D12Device->GetAdapter();
    gpuParams.device = m_D3D12Device->GetDevice();
    gpuParams.commandQueue = m_CommandQueue->GetQueue();
    if (!m_GPUMonitor->Init(gpuParams)) {
        DebugHelper::DebugPrint("m_GPUMonitor 초기화 실패");
        return false;
    }
    return true;
} // LoadPipeline

bool Renderer::LoadSceneRenderTarget(int width, int height) {
    // 오프스크린 렌더 텍스처 매니저 초기화
    RenderTextureManager::InitParams rtmParams;
    rtmParams.device = m_D3D12Device->GetDevice();
    rtmParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    rtmParams.rtvCapacity = RendererState::RTVCapacity;
    rtmParams.dsvCapacity = RendererState::DSVCapacity;

    if (!m_RenderTextureManager->Init(rtmParams)) {
        return false;
    }

    // 실제 메인 씬이 그려질 오프스크린 컬러 + 뎁스 버퍼 지정
    RenderTarget::InitParams rtParams;
    rtParams.device = m_D3D12Device->GetDevice();
    rtParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    rtParams.width = width;
    rtParams.height = height;
    rtParams.colorFormat = RendererState::RTVFormat;
    rtParams.depthFormat = DXGI_FORMAT_D32_FLOAT;

    if (!m_SceneRenderTarget->Init(rtParams)) {
        return false;
    }

    m_SceneCamera->Init(SharedCommons::DEFAULT_FOV, (float)SharedCommons::SCREEN_WIDTH / (float)SharedCommons::SCREEN_HEIGHT,
        SharedCommons::SCREEN_NEAR, SharedCommons::SCREEN_DEPTH);
    m_MasterCamera->Init(SharedCommons::DEFAULT_FOV,
        (float)SharedCommons::SCREEN_WIDTH / (float)SharedCommons::SCREEN_HEIGHT,
        SharedCommons::SCREEN_NEAR, SharedCommons::SCREEN_DEPTH);

    m_MasterCamera->SetPosition(m_SceneCamera->GetPosition());
    m_MasterCamera->SetRotation(m_SceneCamera->GetRotation());
    m_MasterCamera->SetFov(m_SceneCamera->GetFov());
    m_MasterCamera->SetAspect(m_SceneCamera->GetAspect());
    m_MasterCamera->SetNear(m_SceneCamera->GetNear());
    m_MasterCamera->SetFar(m_SceneCamera->GetFar());
    m_MasterCamera->Update();

    m_DirectionalLight->Init();

    CascadedShadowMap::InitParams csmInit;
    csmInit.cascadeCount = SharedCommons::CASCADES_COUNT;
    csmInit.shadowMapSize = SharedCommons::SHADOWMAP_WIDTH;
    csmInit.splitLambda = SharedCommons::SPLIT_LAMBDA;
    if (!m_CascadedShadowMap->Init(csmInit)) {
        DebugHelper::DebugPrint("m_CascadedShadowMap 초기화 실패");
        return false;
    }

    if (!m_RendererState->Init(m_D3D12Device->GetDevice())) {
        DebugHelper::DebugPrint("m_RendererState 초기화 실패");
        return false;
    }

    return true;
} // LoadSceneRenderTarget

bool Renderer::LoadAssets(HWND hwnd) {
	auto device = m_D3D12Device->GetDevice();
	auto commandQueue = m_CommandQueue->GetQueue();

    PSOManager::InitParams psoInitParams;
    psoInitParams.device = device;
    psoInitParams.rtvFormat = RendererState::RTVFormat;
    psoInitParams.dsvFormat = DXGI_FORMAT_D32_FLOAT;

    if (!m_PSOManager->Init(psoInitParams)) {
        DebugHelper::DebugPrint("PSOManager 초기화 실패");
        return false;
    }

    TextureManager::InitParams texInit;
    texInit.device = device;
    texInit.commandQueue = commandQueue;
    texInit.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    texInit.hwnd = nullptr;

    if (!m_TextureManager->Init(texInit)) {
        DebugHelper::DebugPrint("TextureManager Init 실패");
        return false;
    }

    Sponza::InitParams sponzaInitParams;
    sponzaInitParams.device = device;
    sponzaInitParams.commandQueue = commandQueue;
    sponzaInitParams.textureManager = m_TextureManager;
    sponzaInitParams.path = SharedCommons::SPONZA_PATH;
    sponzaInitParams.heapAllocator = m_sharedDescriptorAllocator.get();
    sponzaInitParams.rootSignature = m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_GPU_SPONZA_SIG);
    sponzaInitParams.debugRootSignature = m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_DEBUG_AABB_SIG);
    sponzaInitParams.psoSolidCull = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SPONZA_SOLID_CULL)->GetPSO();
    sponzaInitParams.psoSolidNoCull = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SPONZA_SOLID_NO_CULL)->GetPSO();
    sponzaInitParams.psoWireCull = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SPONZA_WIRE_CULL)->GetPSO();
    sponzaInitParams.psoWireNoCull = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SPONZA_WIRE_NO_CULL)->GetPSO();
    sponzaInitParams.psoDepthSolid = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_DEPTH_SOLID_CULL)->GetPSO();
    sponzaInitParams.psoDepthAlpha = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_DEPTH_ALPHA_NO_CULL)->GetPSO();
    sponzaInitParams.psoShadowSolid = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SHADOW_SOLID_CULL)->GetPSO();
    sponzaInitParams.psoShadowAlpha = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_SHADOW_ALPHA_NO_CULL)->GetPSO();
    sponzaInitParams.psoCSMSolid = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_CSM_SOLID_CULL)->GetPSO();
    sponzaInitParams.psoCSMAlpha = m_PSOManager->GetPSO(SharedCommons::KEY_GPU_CSM_ALPHA_NO_CULL)->GetPSO();
    sponzaInitParams.psoDebug = m_PSOManager->GetPSO(SharedCommons::KEY_DEBUG_AABB_PSO)->GetPSO();
    if (!m_Sponza->Init(sponzaInitParams)) {
        DebugHelper::DebugPrint("Sponza 모델 초기화 실패");
        return false;
    }

    RendererDebugger::InitParams debuggerInitParams = {};
    debuggerInitParams.device = device;
    debuggerInitParams.rendererState = m_RendererState.get();
    debuggerInitParams.renderTextureManager = m_RenderTextureManager.get();
    debuggerInitParams.swapChain = m_SwapChain.get();
    debuggerInitParams.psoManager = m_PSOManager.get();
    debuggerInitParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    debuggerInitParams.sceneCamera = m_SceneCamera.get();
    debuggerInitParams.masterCamera = m_MasterCamera.get();
    debuggerInitParams.occlusionCuller = m_OcclusionCuller.get();
    debuggerInitParams.sponza = m_Sponza.get();
    if (!m_RendererDebugger->Init(debuggerInitParams)) {
        DebugHelper::DebugPrint("씬 카메라 절두체 버퍼 생성 실패");
        return false;
    }

    FrustumCuller::InitParams frumInit;
    frumInit.device = device;
	frumInit.maxMainCount = m_Sponza->GetMainIndirectCount();
	frumInit.maxVaseCount = m_Sponza->GetVaseIndirectCount();
    frumInit.rootSig = m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_CULLING_SIG);
	frumInit.pso = m_PSOManager->GetPSO(SharedCommons::KEY_CULLING_CS)->GetPSO();
    frumInit.heapAllocator = m_sharedDescriptorAllocator.get();
    if (!m_FrustumCuller->Init(frumInit)) {
        DebugHelper::DebugPrint("m_FrustumCuller 초기화 실패");
        return false;
    }

	HierarchicalZBuffer::InitParams hzInit;
	hzInit.device = device;
	hzInit.rootSignature = m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_HIERARCHICAL_Z_SIG);
	hzInit.pso = m_PSOManager->GetPSO(SharedCommons::KEY_HIERARCHICAL_Z_CS_SIG)->GetPSO();
    if (!m_HierarchicalZBuffer->Init(hzInit)) {
        DebugHelper::DebugPrint("HierarchicalZBuffer 초기화 실패");
        return false;
    }

    OcclusionCuller::InitParams occInit;
    occInit.device = device;
    occInit.rootSig = m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_OCCLUSION_CULLING_SIG);
    occInit.pso = m_PSOManager->GetPSO(SharedCommons::KEY_OCCLUSION_CULLING_PSO)->GetPSO();
    occInit.maxMainCount = m_Sponza->GetMainIndirectCount();
    occInit.maxVaseCount = m_Sponza->GetVaseIndirectCount();
    occInit.heapAllocator = m_sharedDescriptorAllocator.get();
    if (!m_OcclusionCuller->Init(occInit)) {
        DebugHelper::DebugPrint("OcclusionCuller 초기화 실패");
        return false;
    }
    return true;
} // LoadAssets

bool Renderer::LoadGUIs(HWND hwnd, std::shared_ptr<ImGuiManager> imGuiManager) {
    m_ImGuiManager = imGuiManager;
    ImGuiManager::InitParams guiParams;
    guiParams.hwnd = hwnd;
    guiParams.device = m_D3D12Device->GetDevice();
    guiParams.numFramesInFlight = RendererState::FrameCount;
    guiParams.rtvFormat = RendererState::RTVFormat;
	guiParams.heapAllocator = m_sharedDescriptorAllocator.get();
    if (!m_ImGuiManager->Init(guiParams)) {
        return false;
    }

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Performance GUI",
        [this]() { OnGUI(); }
    ));

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Directional Light GUI",
        [this]() { m_DirectionalLight->OnGUI(); }
    ));

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Scene Camera GUI",
        [this]() { m_SceneCamera->OnGUI(); }
    ));

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Master Camera GUI",
        [this]() { m_MasterCamera->OnGUI(); }
    ));

 /*   m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Sponza GUI",
        [this]() { m_Sponza->OnGUI(); }
    ));

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Frustum Culler GUI",
        [this]() { m_FrustumCuller->OnGUI(); }
    ));

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Occlusion Culling GUI",
        [this]() { m_OcclusionCuller->OnGUI(); }
    )); */

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Render Textures GUI",
        [this]() { m_RenderTextureManager->OnGUI(); }
    ));

    return true;
} // UpdateGUIs

void Renderer::PopulateCommandList() {
    m_CommandQueue->Reset();
    ID3D12GraphicsCommandList* cmdList = m_CommandQueue->GetList();

    ID3D12DescriptorHeap* heaps[] = { m_sharedDescriptorAllocator->GetHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    m_GPUMonitor->RecordTimestamp(cmdList, 0);

    FrustumPass(cmdList);
 
    OcclusionPhase1Pass(cmdList);
    DepthPass(cmdList);
    ShadowPass(cmdList);
    m_RendererDebugger->DebugCascadedShadowMap(cmdList);
    OcclusionPhase2Pass(cmdList);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // 오프스크린 타겟(씬 렌더 타겟) 세팅
    m_SceneRenderTarget->BeginRender(cmdList, clearColor);

    cmdList->RSSetViewports(1, &m_SwapChain->GetViewport());
    cmdList->RSSetScissorRects(1, &m_SwapChain->GetScissorRect());

    SponzaPass(cmdList);
    //m_RendererDebugger->DebugSceneCameraFrustum(cmdList);

    m_GPUMonitor->RecordTimestamp(cmdList, 1);
    m_SceneRenderTarget->EndRender(cmdList);

    D3D12_RESOURCE_BARRIER preCopyBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_SceneRenderTarget->GetColorResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_SwapChain->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
    };
    cmdList->ResourceBarrier(2, preCopyBarriers);

    cmdList->CopyResource(m_SwapChain->GetCurrentBackBuffer(), m_SceneRenderTarget->GetColorResource());

    D3D12_RESOURCE_BARRIER postCopyBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_SwapChain->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_SceneRenderTarget->GetColorResource(),
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };
    cmdList->ResourceBarrier(2, postCopyBarriers);

    D3D12_CPU_DESCRIPTOR_HANDLE swapChainRTV = m_SwapChain->GetCurrentRTVHandle();
    cmdList->OMSetRenderTargets(1, &swapChainRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &m_SwapChain->GetViewport());
    cmdList->RSSetScissorRects(1, &m_SwapChain->GetScissorRect());

    m_ImGuiManager->RenderDrawData(cmdList);

    CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_SwapChain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(1, &toPresentBarrier);

    m_GPUMonitor->ResolveQueryData(cmdList);
    cmdList->Close();
} // PopulateCommandList

void Renderer::FrustumPass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(0, 255, 255), L"Frustum Culling Pass");

    FrustumCuller::DispatchParams cullParams = {};
    cullParams.cmdList = cmdList;
    cullParams.instanceDataBuffer = m_Sponza->GetInstanceDataBuffer();
    cullParams.instanceDescIndex = m_Sponza->GetInstanceDataDescriptorIndex();
    cullParams.masterIndirectDescriptorIndex = m_Sponza->GetMasterIndirectDescriptorIndex();
    cullParams.mainIndirectBuffer = m_Sponza->GetMainIndirectBuffer();
    cullParams.vaseIndirectBuffer = m_Sponza->GetVaseIndirectBuffer();
    m_FrustumCuller->Dispatch(cullParams);
    //m_FrustumCuller->ReadbackToCPU(cmdList);

    PIXEndEvent(cmdList);
} // FrustumPass

void Renderer::OcclusionPhase1Pass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(255, 165, 0), L"Occlusion Culling Phase 1");

    auto hizTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE).get();
    hizTexture->Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    OcclusionCuller::DispatchPhase1Params params = {};
    params.cmdList = cmdList;
    params.hasPreviousHiz = m_HierarchicalZBuffer->IshasValidHiz();
    params.previousHizTextureDescIndex = hizTexture->GetSRVIndex();
    params.frustumMainCommandsDescIndex = m_FrustumCuller->GetMainVisibleCommandsSRVIndex();
    params.frustumVaseCommandsDescIndex = m_FrustumCuller->GetVaseVisibleCommandsSRVIndex();
    params.frustumMainCountDescIndex = m_FrustumCuller->GetMainCounterSRVIndex();
    params.frustumVaseCountDescIndex = m_FrustumCuller->GetVaseCounterSRVIndex();
    params.meshInstanceDataDescIndex = m_Sponza->GetInstanceDataDescriptorIndex();

    m_OcclusionCuller->DispatchPhase1(params);
    PIXEndEvent(cmdList);
} // OcclusionPhase1Pass

void Renderer::DepthPass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(0, 0, 255), L"Depth Recording Pass");
    auto depthTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_DEPTH_RENDER_TEXTURE).get();

    depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    depthTexture->ClearDepth(cmdList, 0.0f, 0);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthTexture->GetDSVHandle();
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
    cmdList->RSSetViewports(1, &m_SwapChain->GetViewport());
    cmdList->RSSetScissorRects(1, &m_SwapChain->GetScissorRect());

    Sponza::SubmitIndirectParams submitParams;
    submitParams.cmdList = cmdList;
    submitParams.frameConstantsGPUAddress = m_RendererState->GetSceneFrameCBGPUVirtualAddress();
    submitParams.lightConstantsGPUAddress = m_RendererState->GetLightCBGPUVirtualAddress();
    submitParams.shadowMapDescriptorIndex = UINT_MAX;
    submitParams.mainVisibleCommandsBuffer = m_OcclusionCuller->GetFinalMainCommandsBuffer();
    submitParams.mainCounterBuffer = m_OcclusionCuller->GetFinalMainCounterBuffer();
    submitParams.vaseVisibleCommandsBuffer = m_OcclusionCuller->GetFinalVaseCommandsBuffer();
    submitParams.vaseCounterBuffer = m_OcclusionCuller->GetFinalVaseCounterBuffer();
    submitParams.type = Sponza::SubmitIndirectType::Depth;

    m_Sponza->SubmitIndirect(submitParams);
    depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    PIXEndEvent(cmdList);

    PIXBeginEvent(cmdList, PIX_COLOR(0, 255, 0), L"Hi-Z Build Pass");
    HierarchicalZBuffer::BuildParams hizParams;
    hizParams.cmdList = cmdList;
    hizParams.depthTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_DEPTH_RENDER_TEXTURE).get();
    hizParams.hizTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE).get();
    m_HierarchicalZBuffer->Build(hizParams);
    PIXEndEvent(cmdList);

    //m_RendererDebugger->DebugHiZRenderTextures(cmdList);
} // DepthPass

void Renderer::ShadowPass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(180, 120, 40), L"Cascaded Shadow Map Pass");

    auto shadowTexture = m_RenderTextureManager->GetRenderTexture(
        SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE);
    if (!shadowTexture) {
        PIXEndEvent(cmdList);
        return;
    }

    shadowTexture->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(shadowTexture->GetWidth());
    viewport.Height = static_cast<float>(shadowTexture->GetHeight());
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = {
        0, 0,
        static_cast<LONG>(shadowTexture->GetWidth()),
        static_cast<LONG>(shadowTexture->GetHeight())
    };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    const UINT cascadeCount = m_CascadedShadowMap->GetCascadeCount();
    for (UINT cascade = 0; cascade < cascadeCount; ++cascade) {
        D3D12_CPU_DESCRIPTOR_HANDLE sliceDsv = shadowTexture->GetDSVHandle(cascade);
        cmdList->ClearDepthStencilView(sliceDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &sliceDsv);

        Sponza::SubmitIndirectParams submitParams;
        submitParams.cmdList = cmdList;
        submitParams.frameConstantsGPUAddress = m_RendererState->GetFrameCBGPUVirtualAddress();
        submitParams.lightConstantsGPUAddress = m_RendererState->GetLightCBGPUVirtualAddress();
        submitParams.cascadeIndex = cascade;
        submitParams.shadowMapDescriptorIndex = UINT_MAX;
        submitParams.mainVisibleCommandsBuffer = m_Sponza->GetMainIndirectBuffer();
        submitParams.mainCounterBuffer = nullptr;
        submitParams.vaseVisibleCommandsBuffer = m_Sponza->GetVaseIndirectBuffer();
        submitParams.vaseCounterBuffer = nullptr;
        submitParams.type = Sponza::SubmitIndirectType::CSMShadow;

        m_Sponza->SubmitIndirect(submitParams);
    }

    shadowTexture->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    PIXEndEvent(cmdList);
} // ShadowPass

void Renderer::OcclusionPhase2Pass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(255, 100, 0), L"Occlusion Culling Phase 2");

    auto hizTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE).get();

    OcclusionCuller::DispatchPhase2Params params = {};
    params.cmdList = cmdList;

    params.currentHizTextureDescIndex = hizTexture->GetSRVIndex();
    params.meshInstanceDataDescIndex = m_Sponza->GetInstanceDataDescriptorIndex();

    if (m_HierarchicalZBuffer->IshasValidHiz())
    {
        m_OcclusionCuller->DispatchPhase2(params);
    }

    m_HierarchicalZBuffer->OnHiZ();
    //m_OcclusionCuller->ReadbackToCPU(cmdList);

    PIXEndEvent(cmdList);
} // OcclusionPhase2Pass

void Renderer::SponzaPass(ID3D12GraphicsCommandList* cmdList) {
    // 스폰자 씬 렌더링
    PIXBeginEvent(cmdList, PIX_COLOR(255, 0, 0), L"Sponza Indirect Render Pass");

    auto csmTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE);


    Sponza::SubmitIndirectParams submitParams;
    submitParams.cmdList = cmdList;
    submitParams.frameConstantsGPUAddress = m_RendererState->GetFrameCBGPUVirtualAddress();
    submitParams.lightConstantsGPUAddress = m_RendererState->GetLightCBGPUVirtualAddress();
    submitParams.shadowMapDescriptorIndex = m_RenderTextureManager->GetRenderTexture(
        SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE)->GetSRVIndex();
    submitParams.csmShadowMapDescriptorIndex = csmTexture ? csmTexture->GetSRVIndex() : UINT_MAX;
    submitParams.mainVisibleCommandsBuffer = m_OcclusionCuller->GetFinalMainCommandsBuffer();
    submitParams.mainCounterBuffer = m_OcclusionCuller->GetFinalMainCounterBuffer();
    submitParams.vaseVisibleCommandsBuffer = m_OcclusionCuller->GetFinalVaseCommandsBuffer();
    submitParams.vaseCounterBuffer = m_OcclusionCuller->GetFinalVaseCounterBuffer();
    submitParams.type = Sponza::SubmitIndirectType::General;

    m_Sponza->SubmitIndirect(submitParams);

    PIXEndEvent(cmdList);

    //m_RendererDebugger->DebugBoundingBox(cmdList);

} // SponzaPass

void Renderer::OnGUI() {
    const FrameParams& currentFrameParams = m_RendererState->GetCurrentFrameParams();
    ImGui::Text("Camera Mode: %s", m_RendererState->IsUsingMasterCamera() ? "Master View" : "Scene View");
    ImGui::Text("F2: Toggle Scene/Master View");
    ImGui::Text("Scene View: WASD/Z/X + LMB");
    ImGui::Text("Master View: Arrow/C/V + LMB");
    ImGui::Text("Master View Scene Camera: WASD/Z/X + RMB");
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ HARDWARE ]");
    ImGui::Text("CPU: %s", currentFrameParams.cpuName.c_str());
    ImGui::Text("GPU: %s", m_GPUMonitor->GetName().c_str());
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ METRICS ]");
    ImGui::Text("FPS: %d (%.2f ms)", currentFrameParams.fps, currentFrameParams.deltaTime * 1000.0f);
    ImGui::Text("CPU Usage: %ld %%", currentFrameParams.cpuPercentage);

    float vramUsage = m_GPUMonitor->GetVRAMUsageMB();
    float vramTotal = m_GPUMonitor->GetVRAMTotalMB();
    ImGui::Text("VRAM: %.1f MB / %.1f MB", vramUsage, vramTotal);
    float progress = (vramTotal > 0.0f) ? (vramUsage / vramTotal) : 0.0f;
    ImGui::ProgressBar(progress, ImVec2(200.0f, 0.0f));
    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[ GPU PROFILING ]");

    double passTime = m_GPUMonitor->GetTimeMs(0, 1);
    ImGui::Text("Pass: %.3f ms", passTime);
    ImGui::ProgressBar(static_cast<float>(passTime / 16.6), ImVec2(200.0f, 0.0f), "");
    ImGui::Separator();
} // OnGUI
