#include "Pch.h"
#include "Renderer.h"
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
using namespace DirectX;
using Microsoft::WRL::ComPtr;

Renderer::Renderer()
    : m_currentFrameParams{}, m_useMasterCamera(false), m_sceneCameraFrustumVBV{},
    m_sceneCameraFrustumMappedData(nullptr) {
    m_D3D12Device = std::make_unique<D3D12Device>();
    m_CommandQueue = std::make_unique<CommandQueue>();
    m_SwapChain = std::make_unique<D3D12SwapChain>();
    m_SceneRenderTarget = std::make_unique<RenderTarget>();
    m_RendererState = std::make_unique<RendererState>();

    // 컴포넌트 및 매니저 객체 생성
    m_RenderQueue = std::make_unique<RenderQueue>();
    m_SceneCamera = std::make_unique<Camera>();
    m_MasterCamera = std::make_unique<Camera>();
    m_FrustumCuller = std::make_unique<FrustumCuller>();
	m_HierarchicalZBuffer = std::make_unique<HierarchicalZBuffer>();
    m_OcclusionCuller = std::make_unique<OcclusionCuller>();
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
    m_OcclusionCuller.reset();
    m_HierarchicalZBuffer.reset();
	m_FrustumCuller.reset();
    m_Sponza.reset();
    m_TextureManager.reset();
    m_PSOManager.reset();

    // LoadSceneRenderTarget 해제
    m_SceneRenderTarget.reset();
    m_RenderTextureManager.reset();

    if (m_sceneCameraFrustumBuffer) {
        m_sceneCameraFrustumBuffer->Unmap(0, nullptr);
        m_sceneCameraFrustumBuffer.Reset();
    }
    m_sceneCameraFrustumMappedData = nullptr;

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
    m_currentFrameParams = frameParams;
    bool enteredMasterCamera = false;

    if (frameParams.toggleCameraMode) {
        m_useMasterCamera = !m_useMasterCamera;

        if (m_useMasterCamera) {
            enteredMasterCamera = true;
            m_MasterCamera->SetPosition(m_SceneCamera->GetPosition());
            m_MasterCamera->SetRotation(m_SceneCamera->GetRotation());
            m_MasterCamera->SetFov(m_SceneCamera->GetFov());
            m_MasterCamera->SetAspect(m_SceneCamera->GetAspect());
            m_MasterCamera->SetNear(m_SceneCamera->GetNear());
            m_MasterCamera->SetFar(m_SceneCamera->GetFar());
            m_MasterCamera->Update();
        }
    }

    if (m_useMasterCamera) {
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
    }

    m_FrustumCuller->Frame(m_SceneCamera->GetViewMatrix(), m_SceneCamera->GetStandardZProjectionMatrix());
    m_DirectionalLight->Frame();
    m_GPUMonitor->Frame();

    OcclusionCuller::FrameParams occParams;
    occParams.viewMatrix = m_SceneCamera->GetViewMatrix();
    occParams.projectionMatrix = m_SceneCamera->GetReverseZProjectionMatrix();
    occParams.screenWidth = static_cast<float>(RendererState::ScreenWidth);
    occParams.screenHeight = static_cast<float>(RendererState::ScreenHeight);
    m_OcclusionCuller->Frame(occParams);

    Camera* renderCamera = GetActiveCamera();
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
    m_RendererState->Frame(stateParams);

    RendererState::FrameParams sceneFrameParams = stateParams;
    sceneFrameParams.view = m_SceneCamera->GetViewMatrix();
    sceneFrameParams.projection = m_SceneCamera->GetReverseZProjectionMatrix();
    sceneFrameParams.cameraPosition = m_SceneCamera->GetPosition();
    sceneFrameParams.cameraFov = m_SceneCamera->GetFov();
    m_RendererState->FrameScene(sceneFrameParams);

    UpdateSceneCameraFrustum();
    
    return Render();
} // Frame

bool Renderer::Render() {
    m_ImGuiManager->Render();

    PopulateCommandList();

    // 커맨드 리스트 실행 위임
    m_CommandQueue->Execute();

    // 후면 버퍼를 전면 버퍼로 교체
    m_SwapChain->Present(true);

    // 다음 프레임을 위해 동기화 수행
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
    sponzaInitParams.psoDebug = m_PSOManager->GetPSO(SharedCommons::KEY_DEBUG_AABB_PSO)->GetPSO();
    if (!m_Sponza->Init(sponzaInitParams)) {
        DebugHelper::DebugPrint("Sponza 모델 초기화 실패");
        return false;
    }

    if (!BuildSceneCameraFrustumBuffer()) {
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

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Sponza GUI",
        [this]() { m_Sponza->OnGUI(); }
    ));

    //m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
    //    "Frustum Culler GUI",
    //    [this]() { m_FrustumCuller->OnGUI(); }
    //));

    //m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
    //    "Occlusion Culling GUI",
    //    [this]() { m_OcclusionCuller->OnGUI(); }
    //));

    //m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
    //    "Render Textures GUI",
    //    [this]() { m_RenderTextureManager->OnGUI(); }
    //));

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
    OcclusionPhase2Pass(cmdList);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // 오프스크린 타겟(씬 렌더 타겟) 세팅
    m_SceneRenderTarget->BeginRender(cmdList, clearColor);

    cmdList->RSSetViewports(1, &m_SwapChain->GetViewport());
    cmdList->RSSetScissorRects(1, &m_SwapChain->GetScissorRect());

    SponzaPass(cmdList);
    DebugSceneCameraFrustum(cmdList);

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

    //DebugHiZRenderTextures(cmdList);
} // DepthPass

void Renderer::ShadowPass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(180, 120, 40), L"Shadow Map Pass");

    auto shadowTexture = m_RenderTextureManager->GetRenderTexture(
        SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE);
    if (!shadowTexture) {
        PIXEndEvent(cmdList);
        return;
    }

    shadowTexture->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    shadowTexture->ClearDepth(cmdList, 1.0f, 0);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = shadowTexture->GetDSVHandle();
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(shadowTexture->GetWidth());
    viewport.Height = static_cast<float>(shadowTexture->GetHeight());
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = {
        0,
        0,
        static_cast<LONG>(shadowTexture->GetWidth()),
        static_cast<LONG>(shadowTexture->GetHeight())
    };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    Sponza::SubmitIndirectParams submitParams;
    submitParams.cmdList = cmdList;
    submitParams.frameConstantsGPUAddress = m_RendererState->GetFrameCBGPUVirtualAddress();
    submitParams.lightConstantsGPUAddress = m_RendererState->GetLightCBGPUVirtualAddress();
    submitParams.shadowMapDescriptorIndex = shadowTexture->GetSRVIndex();
    submitParams.mainVisibleCommandsBuffer = m_Sponza->GetMainIndirectBuffer();
    submitParams.mainCounterBuffer = nullptr;
    submitParams.vaseVisibleCommandsBuffer = m_Sponza->GetVaseIndirectBuffer();
    submitParams.vaseCounterBuffer = nullptr;
    submitParams.type = Sponza::SubmitIndirectType::Shadow;

    m_Sponza->SubmitIndirect(submitParams);
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

    Sponza::SubmitIndirectParams submitParams;
    submitParams.cmdList = cmdList;
    submitParams.frameConstantsGPUAddress = m_RendererState->GetFrameCBGPUVirtualAddress();
    submitParams.lightConstantsGPUAddress = m_RendererState->GetLightCBGPUVirtualAddress();
    submitParams.shadowMapDescriptorIndex = m_RenderTextureManager->GetRenderTexture(
        SharedCommons::KEY_SHADOW_MAP_RENDER_TEXTURE)->GetSRVIndex();
    submitParams.mainVisibleCommandsBuffer = m_OcclusionCuller->GetFinalMainCommandsBuffer();
    submitParams.mainCounterBuffer = m_OcclusionCuller->GetFinalMainCounterBuffer();
    submitParams.vaseVisibleCommandsBuffer = m_OcclusionCuller->GetFinalVaseCommandsBuffer();
    submitParams.vaseCounterBuffer = m_OcclusionCuller->GetFinalVaseCounterBuffer();
    submitParams.type = Sponza::SubmitIndirectType::General;

    m_Sponza->SubmitIndirect(submitParams);

    PIXEndEvent(cmdList);

    //DebugBoundingBox(cmdList);

} // SponzaPass

void Renderer::OnGUI() {
    ImGui::Text("Camera Mode: %s", m_useMasterCamera ? "Master View" : "Scene View");
    ImGui::Text("F2: Toggle Scene/Master View");
    ImGui::Text("Scene View: WASD/Z/X + LMB");
    ImGui::Text("Master View: Arrow/C/V + LMB");
    ImGui::Text("Master View Scene Camera: WASD/Z/X + RMB");
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ HARDWARE ]");
    ImGui::Text("CPU: %s", m_currentFrameParams.cpuName.c_str());
    ImGui::Text("GPU: %s", m_GPUMonitor->GetName().c_str());
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ METRICS ]");
    ImGui::Text("FPS: %d (%.2f ms)", m_currentFrameParams.fps, m_currentFrameParams.deltaTime * 1000.0f);
    ImGui::Text("CPU Usage: %ld %%", m_currentFrameParams.cpuPercentage);

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

void Renderer::DebugDepthRenderTextures(ID3D12GraphicsCommandList* cmdList, RenderTexture* depthTexture) {
    PIXBeginEvent(cmdList, PIX_COLOR(128, 128, 128), L"Depth Debug Pass");
    {
        auto debugDepthTex = m_RenderTextureManager->CreateRenderTexture(
            "Depth_Debug",
            depthTexture->GetWidth(),
            depthTexture->GetHeight(),
            RenderTexture::RenderTextureType::Normal,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1
        ).get();
        debugDepthTex->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE debugRtvHandle = debugDepthTex->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &debugRtvHandle, FALSE, nullptr);

        cmdList->RSSetViewports(1, &m_SwapChain->GetViewport());
        cmdList->RSSetScissorRects(1, &m_SwapChain->GetScissorRect());
        cmdList->SetGraphicsRootSignature(m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_TRANS_REVERSE_Z_SIG));
        cmdList->SetPipelineState(m_PSOManager->GetPSO(SharedCommons::KEY_TRANS_REVERSE_Z_PSO)->GetPSO());

        const Camera* activeCamera = GetActiveCamera();
        struct { float nearPlane; float farPlane; } cameraClip = { activeCamera->GetNear(), activeCamera->GetFar() };
        cmdList->SetGraphicsRoot32BitConstants(RendererState::DebugCameraClipIndex, 2, &cameraClip, 0);

        cmdList->SetGraphicsRootDescriptorTable(
            RendererState::DebugDepthTexIndex,
            m_sharedDescriptorAllocator->GetGPUHandle(depthTexture->GetSRVIndex()));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        debugDepthTex->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    PIXEndEvent(cmdList);
} // DebugRenderTextures

void Renderer::DebugHiZRenderTextures(ID3D12GraphicsCommandList* cmdList) {
    HierarchicalZBuffer::BuildParams hizParams;
    hizParams.cmdList = cmdList;
    hizParams.depthTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_DEPTH_RENDER_TEXTURE).get();
    hizParams.hizTexture = m_RenderTextureManager->GetRenderTexture(SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE).get();
    PIXBeginEvent(cmdList, PIX_COLOR(128, 255, 128), L"Hi-Z Debug Pass");
    {
        auto hizTexture = hizParams.hizTexture;

        auto hizDebugTex = m_RenderTextureManager->CreateRenderTexture(
            "HiZ_Depth_Debug",
            hizTexture->GetWidth(),
            hizTexture->GetHeight(),
            RenderTexture::RenderTextureType::Normal,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1
        ).get();

        hizTexture->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        hizDebugTex->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE hizDebugRtv = hizDebugTex->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &hizDebugRtv, FALSE, nullptr);

        D3D12_VIEWPORT hizViewport = { 0.0f, 0.0f, static_cast<float>(hizTexture->GetWidth()), static_cast<float>(hizTexture->GetHeight()), 0.0f, 1.0f };
        D3D12_RECT hizScissor = { 0, 0, static_cast<LONG>(hizTexture->GetWidth()), static_cast<LONG>(hizTexture->GetHeight()) };
        cmdList->RSSetViewports(1, &hizViewport);
        cmdList->RSSetScissorRects(1, &hizScissor);

        cmdList->SetGraphicsRootSignature(m_PSOManager->GetID3D12RootSignature(SharedCommons::KEY_TRANS_REVERSE_Z_SIG));
        cmdList->SetPipelineState(m_PSOManager->GetPSO(SharedCommons::KEY_TRANS_REVERSE_Z_PSO)->GetPSO());

        const Camera* activeCamera = GetActiveCamera();
        struct { float nearPlane; float farPlane; } cameraClip = { activeCamera->GetNear(), activeCamera->GetFar() };
        cmdList->SetGraphicsRoot32BitConstants(RendererState::DebugCameraClipIndex, 2, &cameraClip, 0);

        cmdList->SetGraphicsRootDescriptorTable(
            RendererState::DebugDepthTexIndex,
            m_sharedDescriptorAllocator->GetGPUHandle(hizTexture->GetSRVIndex()));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        hizDebugTex->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    PIXEndEvent(cmdList);
} // DebugHiZRenderTextures

void Renderer::DebugBoundingBox(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(0, 255, 0), L"Debug AABB Pass");

    Sponza::RenderDebugParams debugParams;
    debugParams.cmdList = cmdList;
    debugParams.frameConstantsGPUAddress = m_RendererState->GetFrameCBGPUVirtualAddress();
    debugParams.mainCmdBuffer = m_OcclusionCuller->GetFinalMainCommandsBuffer();
    debugParams.mainCounter = m_OcclusionCuller->GetFinalMainCounterBuffer();
    debugParams.vaseCmdBuffer = m_OcclusionCuller->GetFinalVaseCommandsBuffer();
    debugParams.vaseCounter = m_OcclusionCuller->GetFinalVaseCounterBuffer();

    m_Sponza->RenderDebugAABB(debugParams);

    PIXEndEvent(cmdList);
} // DebugBoundingBox

Camera* Renderer::GetActiveCamera() {
    return m_useMasterCamera ? m_MasterCamera.get() : m_SceneCamera.get();
} // GetActiveCamera

const Camera* Renderer::GetActiveCamera() const {
    return m_useMasterCamera ? m_MasterCamera.get() : m_SceneCamera.get();
} // GetActiveCamera

bool Renderer::BuildSceneCameraFrustumBuffer() {
    if (!m_D3D12Device || !m_D3D12Device->GetDevice()) {
        return false;
    }

    const UINT vertexCount = 24;
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(DebugLineVertex) * vertexCount);

    if (FAILED(m_D3D12Device->GetDevice()->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_sceneCameraFrustumBuffer)))) {
        return false;
    }

    if (FAILED(m_sceneCameraFrustumBuffer->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&m_sceneCameraFrustumMappedData)))) {
        m_sceneCameraFrustumBuffer.Reset();
        return false;
    }

    m_sceneCameraFrustumVBV.BufferLocation = m_sceneCameraFrustumBuffer->GetGPUVirtualAddress();
    m_sceneCameraFrustumVBV.SizeInBytes = sizeof(DebugLineVertex) * vertexCount;
    m_sceneCameraFrustumVBV.StrideInBytes = sizeof(DebugLineVertex);
    UpdateSceneCameraFrustum();
    return true;
} // BuildSceneCameraFrustumBuffer

void Renderer::UpdateSceneCameraFrustum() {
    if (!m_sceneCameraFrustumMappedData || !m_SceneCamera) {
        return;
    }

    const XMMATRIX viewProjection = m_SceneCamera->GetViewMatrix()
        * m_SceneCamera->GetStandardZProjectionMatrix();
    const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, viewProjection);

    const XMFLOAT3 ndcCorners[8] = {
        { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }
    };

    XMFLOAT3 worldCorners[8] = {};
    for (int i = 0; i < 8; ++i) {
        const XMVECTOR ndc = XMLoadFloat3(&ndcCorners[i]);
        XMStoreFloat3(&worldCorners[i], XMVector3TransformCoord(ndc, inverseViewProjection));
    }

    const XMFLOAT3 nearColor = { 1.0f, 0.2f, 0.1f };
    const XMFLOAT3 farColor = { 1.0f, 0.8f, 0.1f };

    auto writeLine = [this](UINT index, const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& color) {
        m_sceneCameraFrustumMappedData[index * 2] = { a, color };
        m_sceneCameraFrustumMappedData[index * 2 + 1] = { b, color };
    };

    writeLine(0, worldCorners[0], worldCorners[1], nearColor);
    writeLine(1, worldCorners[1], worldCorners[2], nearColor);
    writeLine(2, worldCorners[2], worldCorners[3], nearColor);
    writeLine(3, worldCorners[3], worldCorners[0], nearColor);
    writeLine(4, worldCorners[4], worldCorners[5], farColor);
    writeLine(5, worldCorners[5], worldCorners[6], farColor);
    writeLine(6, worldCorners[6], worldCorners[7], farColor);
    writeLine(7, worldCorners[7], worldCorners[4], farColor);
    writeLine(8, worldCorners[0], worldCorners[4], nearColor);
    writeLine(9, worldCorners[1], worldCorners[5], nearColor);
    writeLine(10, worldCorners[2], worldCorners[6], nearColor);
    writeLine(11, worldCorners[3], worldCorners[7], nearColor);
} // UpdateSceneCameraFrustum

void Renderer::DebugSceneCameraFrustum(ID3D12GraphicsCommandList* cmdList) {
    if (!m_useMasterCamera || !m_sceneCameraFrustumBuffer || !m_PSOManager) {
        return;
    }

    ID3D12RootSignature* rootSignature = m_PSOManager->GetID3D12RootSignature(
        SharedCommons::KEY_DEBUG_LINE_SIG);
    D3D12PipelineState* pipelineState = m_PSOManager->GetPSO(
        SharedCommons::KEY_DEBUG_LINE_PSO);
    if (!rootSignature || !pipelineState) {
        return;
    }

    PIXBeginEvent(cmdList, PIX_COLOR(255, 80, 40), L"Scene Camera Frustum Debug");
    cmdList->SetGraphicsRootSignature(rootSignature);
    cmdList->SetPipelineState(pipelineState->GetPSO());
    cmdList->SetGraphicsRootConstantBufferView(
        RendererState::DebugLineFrameIndex,
        m_RendererState->GetFrameCBGPUVirtualAddress());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_sceneCameraFrustumVBV);
    cmdList->DrawInstanced(24, 1, 0, 0);
    PIXEndEvent(cmdList);
} // DebugSceneCameraFrustum
