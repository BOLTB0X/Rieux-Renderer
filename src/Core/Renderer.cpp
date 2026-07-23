#include "Pch.h"
#include "Renderer.h"
// D3D12
#include "D3D12/D3D12Device.h"
#include "D3D12/CommandQueue.h"
#include "D3D12/D3D12SwapChain.h"
// Components
#include "DescriptorHeapAllocator.h"
#include "RenderQueue.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "GPUMonitor.h"
// Managers
#include "RenderTextureManager.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
// Utils
#include "DebugHelper.h"
#include "SharedCommons.h"
#include "SharedCBs.h"
#include "FunctionWidget.h"

using namespace DebugHelper;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

Renderer::Renderer()
    : m_viewport{}, m_scissorRect{}, m_rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
    m_mappedFrameCB(nullptr), m_mappedLightCB(nullptr), m_currentFrameParams{} {

    // 컴포넌트 및 매니저 객체 생성
    m_RenderQueue = std::make_unique<RenderQueue>();
    m_Camera = std::make_unique<Camera>();
    m_DirectionalLight = std::make_unique<DirectionalLight>();
    m_GPUMonitor = std::make_unique<GPUMonitor>();
    m_TextureManager = std::make_shared<TextureManager>();
    m_sharedDescriptorAllocator = std::make_unique<DescriptorHeapAllocator>();

    // 신규 캡슐화 개체 할당
    m_D3D12Device = std::make_unique<D3D12Device>();
    m_CommandQueue = std::make_unique<CommandQueue>();
    m_SwapChain = std::make_unique<D3D12SwapChain>();
    m_RenderTextureManager = std::make_unique<RenderTextureManager>();
} // Renderer

Renderer::~Renderer() {
    Shutdown();
} // ~Renderer

bool Renderer::Init(const InitParams& params) {
    if (!params.hwnd || !params.imGuiManager) {
        return false;
    }

    m_viewport = { 0.0f, 0.0f, static_cast<float>(params.width), static_cast<float>(params.height), 0.0f, 1.0f };
    m_scissorRect = { 0, 0, params.width, params.height };

    // 1. 핵심 D3D12 파이프라인 컴포넌트 로드
    if (!LoadPipeline(params.hwnd, params.width, params.height)) {
        return false;
    }

    // 2. 오프스크린 렌더 타겟 및 텍스처 매니저 초기화
    if (!InitSceneRenderTarget(params.width, params.height)) {
        return false;
    }

    // 3. 에셋 및 셰이더 리소스 로드
    if (!LoadAssets(params.hwnd)) {
        return false;
    }

    // 4. GUI 시스템 초기화
    if (!LoadGUIs(params.hwnd, params.imGuiManager)) {
        return false;
    }

    return true;
} // Init

void Renderer::Shutdown() {
    if (m_CommandQueue) {
        m_CommandQueue->Shutdown(); // 내부에서 GPU 동기화 처리 후 Fence 핸들 해제
    }

    ShutdownCommonCBs();
} // Shutdown

bool Renderer::Frame(const FrameParams& frameParams) {
    m_currentFrameParams = frameParams;

    m_Camera->Frame(frameParams.moveForward, frameParams.moveRight, frameParams.moveUp,
        frameParams.rotationDeltaX, frameParams.rotationDeltaY, frameParams.zoomDelta);
    m_GPUMonitor->Frame();

    return Render();
} // Frame

bool Renderer::Render() {
    UpdateCommonCBs();

    m_ImGuiManager->Render();

    PopulateCommandList();

    // 커맨드 리스트 실행 위임
    m_CommandQueue->Execute();

    // 후면 버퍼를 전면 버퍼로 교체 (VSync 활성화 파라미터 전달)
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
    swapChainParams.frameCount = 2; // 기본 더블 버퍼링 세팅

    if (!m_SwapChain->Init(swapChainParams)) {
        DebugHelper::DebugPrint("D3D12SwapChain 초기화 실패");
        return false;
    }

    // 공유 디스크립터 힙 초기화
    if (!InitSharedDescriptorHeap()) {
        DebugHelper::DebugPrint("InitSharedDescriptorHeap() 실패");
        return false;
    }

    // 공용 상수 버퍼 초기화
    if (!InitCommonCBs()) {
        DebugHelper::DebugPrint("InitCommonCBs() 실패");
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

bool Renderer::InitSharedDescriptorHeap() {
    return m_sharedDescriptorAllocator->Init(
        m_D3D12Device->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererState::KSharedHeapCapacity, true);
} // InitSharedDescriptorHeap

bool Renderer::InitSceneRenderTarget(int width, int height) {
    // 1. 오프스크린 렌더 텍스처 매니저 초기화
    RenderTextureManager::InitParams rtmParams;
    rtmParams.device = m_D3D12Device->GetDevice();
    rtmParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    rtmParams.rtvCapacity = 64;
    rtmParams.dsvCapacity = 16;

    if (!m_RenderTextureManager->Init(rtmParams)) {
        return false;
    }

    // 2. 실제 메인 씬이 그려질 오프스크린 컬러 + 뎁스 버퍼 지정
    RenderTarget::InitParams rtParams;
    rtParams.device = m_D3D12Device->GetDevice();
    rtParams.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    rtParams.width = width;
    rtParams.height = height;
    rtParams.colorFormat = m_rtvFormat;
    rtParams.depthFormat = DXGI_FORMAT_D32_FLOAT;

    return m_SceneRenderTarget.Init(rtParams);
} // InitSceneRenderTarget

bool Renderer::LoadAssets(HWND hwnd) {
    // --------------------------------------------------
    // 루트 시그니처 조립
    // --------------------------------------------------
    D3D12_DESCRIPTOR_RANGE cbvRange = {};
    cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvRange.NumDescriptors = RendererState::KReservedDescriptorCount;
    cbvRange.BaseShaderRegister = 0;
    cbvRange.RegisterSpace = 0;
    cbvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = RendererState::KSharedHeapCapacity - RendererState::KReservedDescriptorCount;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvRange;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &staticSampler;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        if (error) {
            DebugHelper::DebugPrint(std::string("루트 시그니처 직렬화 실패: ") + static_cast<const char*>(error->GetBufferPointer()));
        }
        return false;
    }
    m_D3D12Device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

    // 텍스처 매니저 의존성 주입
    TextureManager::InitParams texInit;
    texInit.device = m_D3D12Device->GetDevice();
    texInit.commandQueue = m_CommandQueue->GetQueue();
    texInit.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    texInit.hwnd = nullptr;

    if (!m_TextureManager->Init(texInit)) {
        DebugHelper::DebugPrint("TextureManager Init 실패");
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
    guiParams.rtvFormat = m_rtvFormat;
    if (!m_ImGuiManager->Init(guiParams)) {
        return false;
    }

    m_ImGuiManager->AddWidget(std::make_unique<FunctionWidget>(
        "Performance GUI",
        [this]() { OnGUI(); }
    ));

    return true;
} // UpdateGUIs

bool Renderer::InitCommonCBs() {
    m_Camera->Init(SharedCommons::DEFAULT_FOV,
        (float)SharedCommons::SCREEN_WIDTH / (float)SharedCommons::SCREEN_HEIGHT,
        SharedCommons::SCREEN_NEAR, SharedCommons::SCREEN_DEPTH);
    m_DirectionalLight->Init();

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const UINT frameCBSize = (sizeof(SharedCBs::FrameCB) + 255) & ~255;
    const UINT lightCBSize = (sizeof(SharedCBs::DirectionalLightCB) + 255) & ~255;

    CD3DX12_RESOURCE_DESC frameCBDesc = CD3DX12_RESOURCE_DESC::Buffer(frameCBSize);
    m_D3D12Device->GetDevice()->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &frameCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_frameCB));
    m_frameCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedFrameCB));

    CD3DX12_RESOURCE_DESC lightCBDesc = CD3DX12_RESOURCE_DESC::Buffer(lightCBSize);
    m_D3D12Device->GetDevice()->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &lightCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_lightCB));
    m_lightCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedLightCB));

    UINT frameCBVIndex = m_sharedDescriptorAllocator->Allocate();
    UINT lightCBVIndex = m_sharedDescriptorAllocator->Allocate();
    if (frameCBVIndex != RendererState::KFrameCBVIndex || lightCBVIndex != RendererState::KLightCBVIndex) {
        DebugHelper::DebugPrint("공용 CBV가 예약된 인덱스(0,1)에 할당되지 않음 - 힙 할당 순서 확인 필요");
        return false;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC frameCbvDesc = {};
    frameCbvDesc.BufferLocation = m_frameCB->GetGPUVirtualAddress();
    frameCbvDesc.SizeInBytes = frameCBSize;
    m_D3D12Device->GetDevice()->CreateConstantBufferView(&frameCbvDesc, m_sharedDescriptorAllocator->GetCPUHandle(RendererState::KFrameCBVIndex));

    D3D12_CONSTANT_BUFFER_VIEW_DESC lightCbvDesc = {};
    lightCbvDesc.BufferLocation = m_lightCB->GetGPUVirtualAddress();
    lightCbvDesc.SizeInBytes = lightCBSize;
    m_D3D12Device->GetDevice()->CreateConstantBufferView(&lightCbvDesc, m_sharedDescriptorAllocator->GetCPUHandle(RendererState::KLightCBVIndex));

    return true;
} // InitCommonCBs

void Renderer::UpdateCommonCBs() {
    m_Camera->Update();
    m_DirectionalLight->Update();

    SharedCBs::FrameCB frameData;
    frameData.view = XMMatrixTranspose(m_Camera->GetViewMatrix());
    frameData.projection = XMMatrixTranspose(m_Camera->GetProjectionMatrix());
    frameData.cameraPosition = m_Camera->GetPosition();
    frameData.cameraFov = m_Camera->GetFov();
    memcpy(m_mappedFrameCB, &frameData, sizeof(SharedCBs::FrameCB));

    SharedCBs::DirectionalLightCB lightData;
    lightData.direction = m_DirectionalLight->GetDirection();
    lightData.ambient = m_DirectionalLight->GetAmbient();
    lightData.diffuse = m_DirectionalLight->GetDiffuse();
    lightData.lookAt = m_DirectionalLight->GetLookAt();
    lightData.lightViewMatrix = XMMatrixTranspose(m_DirectionalLight->GetViewMatrix());
    lightData.lightProjectionMatrix = XMMatrixTranspose(m_DirectionalLight->GetProjection());
    memcpy(m_mappedLightCB, &lightData, sizeof(SharedCBs::DirectionalLightCB));
} // UpdateCommonCBs

void Renderer::ShutdownCommonCBs() {
    if (m_frameCB) {
        m_frameCB->Unmap(0, nullptr);
        m_frameCB.Reset();
    }
    if (m_lightCB) {
        m_lightCB->Unmap(0, nullptr);
        m_lightCB.Reset();
    }
} // ShutdownCommonCBs

void Renderer::PopulateCommandList() {
    // CommandQueue 객체를 이용해 Allocator 및 CommandList 일괄 리셋 진행
    m_CommandQueue->Reset();
    ID3D12GraphicsCommandList* cmdList = m_CommandQueue->GetList();

    // 초기 파이프라인 스테이트 설정
    if (m_pipelineState) {
        cmdList->SetPipelineState(m_pipelineState.Get());
    }

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 공유 힙 바인딩
    ID3D12DescriptorHeap* heaps[] = { m_sharedDescriptorAllocator->GetHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    cmdList->SetGraphicsRootDescriptorTable(0, m_sharedDescriptorAllocator->GetGPUHandle(RendererState::KFrameCBVIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_sharedDescriptorAllocator->GetGPUHandle(RendererState::KReservedDescriptorCount));

    // -----------------------------------------------------------------
    // STEP 1: 오프스크린 SceneRenderTarget 렌더링 패스
    // -----------------------------------------------------------------
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // 내부적으로 Barrier(SRV->RTV), OMSetRenderTargets, Clear 처리가 제어됨
    m_SceneRenderTarget.BeginRender(cmdList, clearColor);

    m_GPUMonitor->RecordTimestamp(cmdList, 0);
    // TODO(Sponza): 메쉬 그리기 구간 (IASet 버퍼 및 DrawIndexedInstanced)
    m_GPUMonitor->RecordTimestamp(cmdList, 1);

    // 내부적으로 Barrier(RTV->SRV) 처리되어 셰이더 읽기 가능 상태로 반환
    m_SceneRenderTarget.EndRender(cmdList);

    // -----------------------------------------------------------------
    // STEP 2: 화면 출력용 SwapChain 백버퍼 쓰기 패스
    // -----------------------------------------------------------------
    // 백버퍼의 상태를 PRESENT -> RENDER_TARGET 으로 직접 배리어 전환
    CD3DX12_RESOURCE_BARRIER toRenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_SwapChain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmdList->ResourceBarrier(1, &toRenderTargetBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE swapChainRTV = m_SwapChain->GetCurrentRTVHandle();
    cmdList->OMSetRenderTargets(1, &swapChainRTV, FALSE, nullptr);

    // 뷰포트 및 가위 사각형 설정 매칭
    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);

    // 오프스크린 렌더 텍스처 결과를 화면 백버퍼로 다이렉트 복사
    cmdList->CopyResource(m_SwapChain->GetCurrentBackBuffer(), m_SceneRenderTarget.GetColorResource());

    // UI(ImGui) 오버레이 드로우
    m_ImGuiManager->RenderDrawData(cmdList);

    // 백버퍼의 상태를 RENDER_TARGET -> PRESENT 로 롤백
    CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_SwapChain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    cmdList->ResourceBarrier(1, &toPresentBarrier);

    m_GPUMonitor->ResolveQueryData(cmdList);

    cmdList->Close();
} // PopulateCommandList

void Renderer::OnGUI() {
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

    double opaquePassTime = m_GPUMonitor->GetTimeMs(0, 1);
    ImGui::Text("Sponza Opaque Pass: %.3f ms", opaquePassTime);
    ImGui::ProgressBar(static_cast<float>(opaquePassTime / 16.6), ImVec2(200.0f, 0.0f), "");
} // OnGUI