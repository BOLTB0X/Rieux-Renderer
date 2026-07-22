#include "Pch.h"
#include "Renderer.h"
// Graphics
#include "Components/Camera.h"
#include "Components/DirectionalLight.h"
#include "Components/GPUMonitor.h"
#include "Components/DescriptorHeapAllocator.h"
#include "Managers/TextureManager.h"
#include "Managers/ImGuiManager.h"
// Util
#include "SharedConstants.h"
#include "SharedStructs.h"
#include "ShaderHelper.h"
#include "DebugHelper.h"
#include "FunctionWidget.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace DirectX;

Renderer::Renderer() : m_viewport{}, m_scissorRect{}, m_rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
m_frameIndex(0), m_rtvDescriptorSize(0), m_fenceValue(0), m_fenceEvent(nullptr) {
    m_Camera = std::make_unique<Camera>();
    m_DirectionalLight = std::make_unique<DirectionalLight>();
    m_GPUMonitor = std::make_unique<GPUMonitor>();
    m_TextureManager = std::make_shared<TextureManager>();
    m_sharedDescriptorAllocator = std::make_unique<DescriptorHeapAllocator>();
    m_mappedFrameCB = nullptr;
    m_mappedLightCB = nullptr;
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

    if (!LoadPipeline(params.hwnd, params.width, params.height)) {
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
    if (m_fenceEvent == nullptr) {
        return;
    }

    WaitForPreviousFrame();
    ShutdownCommonCBs();
    CloseHandle(m_fenceEvent);

    m_fenceEvent = nullptr;
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

    //커맨드 리스트 실행
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 후면 버퍼를 전면 버퍼로 교체
    m_swapChain->Present(1, 0);

    // 다음 프레임을 위해 동기화
    WaitForPreviousFrame();

    return true;
} // Render

bool Renderer::LoadPipeline(HWND hwnd, int width, int height) {
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // 디버그 레이어 활성화
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
        return false;
    }

    // 어뎁터
    ComPtr<IDXGIAdapter1> adapter1;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter1); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter1->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
            break; // 성공하면 탈출
        }
    }

    if (!m_device) {
        DebugHelper::DebugPrint("사용 가능한 DX12 호환 그래픽 장치를 찾지 못함");
        return false;
    }

    adapter1.As(&m_iDXGIAdapter3);

    // 커맨드 큐 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

    // 스왑 체인 생성
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = m_rtvFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain
    );
    swapChain.As(&m_swapChain);
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // RTV(Render Target View) 서술자 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 렌더 타겟(버퍼) 뷰 생성
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < FrameCount; n++) {
        m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
        m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    // 커맨드 할당자 생성
    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));

    // CBV + 텍스처 SRV가 함께 쓸 공유 디스크립터 힙 (CBV/텍스처 순서로 반드시 먼저 준비)
    if (!InitSharedDescriptorHeap()) {
        DebugHelper::DebugPrint("InitSharedDescriptorHeap() failed.");
        return false;
    }

    if (!InitCommonCBs()) {
        DebugHelper::DebugPrint("InitCommonCBs() failed.");
        return false;
    }

    if (!m_GPUMonitor->Init(m_iDXGIAdapter3.Get())) {
        return false;
    }

    return true;
} // LoadPipeline

bool Renderer::InitSharedDescriptorHeap() {
    return m_sharedDescriptorAllocator->Init(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kSharedHeapCapacity, true);
} // InitSharedDescriptorHeap

bool Renderer::LoadAssets(HWND hwnd) {
    // --------------------------------------------------
    // 루트 시그니처: CBV(FrameCB, LightCB) 테이블 + 텍스처 SRV 테이블
    // 삼각형 시절의 root CBV 직접 바인딩 방식을 걷어내고,
    // 공유 디스크립터 힙을 가리키는 테이블 방식으로 전환.
    // --------------------------------------------------
    D3D12_DESCRIPTOR_RANGE cbvRange = {};
    cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvRange.NumDescriptors = kReservedDescriptorCount; // FrameCB(b0), DirectionalLightCB(b1)
    cbvRange.BaseShaderRegister = 0;
    cbvRange.RegisterSpace = 0;
    cbvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Sponza 머티리얼 텍스처가 들어갈 자리. 지금은 지오메트리가 없어 실제로 바인딩하진
    // 않지만, Assimp 연동 시 텍스처 개수만큼 t0..에 순서대로 채워질 예정.
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = kSharedHeapCapacity - kReservedDescriptorCount;
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

    // Sponza 텍스처 샘플링용 기본 static sampler (linear wrap)
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0; // s0
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
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            DebugHelper::DebugPrint(std::string("루트 시그니처 직렬화 실패: ") +
                static_cast<const char*>(error->GetBufferPointer()));
        }
        return false;
    }
    m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

    // 삼각형용 PSO/셰이더/인풋 레이아웃/버텍스 버퍼는 전부 제거.
    // Sponza 지오메트리 + 셰이더가 준비되면 여기서 PSO를 생성할 예정.
    // 그 전까지 m_pipelineState는 nullptr로 두고, 커맨드 리스트도 초기 PSO 없이 생성.
    m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    m_commandList->Close(); // 첫 할당 시 닫아두어야 함

    // 동기화 객체 생성
    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    TextureManager::InitParams texInit;
    texInit.device = m_device.Get();
    texInit.commandQueue = m_commandQueue.Get();
    texInit.sharedDescriptorAllocator = m_sharedDescriptorAllocator.get();
    texInit.hwnd = nullptr; // 필요시 실제 hwnd로

    if (!m_TextureManager->Init(texInit)) {
        DebugHelper::DebugPrint("TextureManager Init 실패");
        return false;
    }
    return true;
} // LoadAssets

bool Renderer::InitCommonCBs() {
    m_Camera->Init(SharedConstants::DEFAULT_FOV,
        (float)SharedConstants::SCREEN_WIDTH / (float)SharedConstants::SCREEN_HEIGHT,
        SharedConstants::SCREEN_NEAR, SharedConstants::SCREEN_DEPTH);
    m_DirectionalLight->Init();

    // 업로드 힙 속성 준비
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    // CBV는 256바이트 정렬이 필요함
    const UINT frameCBSize = (sizeof(SharedStructs::FrameCB) + 255) & ~255;
    const UINT lightCBSize = (sizeof(SharedStructs::DirectionalLightCB) + 255) & ~255;

    CD3DX12_RESOURCE_DESC frameCBDesc = CD3DX12_RESOURCE_DESC::Buffer(frameCBSize);
    m_device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &frameCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_frameCB));
    m_frameCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedFrameCB));

    CD3DX12_RESOURCE_DESC lightCBDesc = CD3DX12_RESOURCE_DESC::Buffer(lightCBSize);
    m_device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &lightCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_lightCB));
    m_lightCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedLightCB));

    // 공유 힙의 예약된 슬롯(0, 1)에 CBV 등록 - 텍스처보다 반드시 먼저 할당돼 있어야 함
    UINT frameCBVIndex = m_sharedDescriptorAllocator->Allocate();
    UINT lightCBVIndex = m_sharedDescriptorAllocator->Allocate();
    if (frameCBVIndex != kFrameCBVIndex || lightCBVIndex != kLightCBVIndex) {
        // 이 시점엔 힙에서 아무것도 할당된 적이 없어야 하므로 항상 0, 1이 나와야 정상.
        // 여기 걸리면 InitCommonCBs()가 InitSharedDescriptorHeap()보다 먼저 불렸거나
        // 다른 곳에서 이미 이 힙을 Allocate()한 것 
        DebugHelper::DebugPrint("공용 CBV가 예약된 인덱스(0,1)에 할당되지 않음 - 힙 할당 순서 확인 필요");
        return false;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC frameCbvDesc = {};
    frameCbvDesc.BufferLocation = m_frameCB->GetGPUVirtualAddress();
    frameCbvDesc.SizeInBytes = frameCBSize;
    m_device->CreateConstantBufferView(&frameCbvDesc, m_sharedDescriptorAllocator->GetCPUHandle(kFrameCBVIndex));

    D3D12_CONSTANT_BUFFER_VIEW_DESC lightCbvDesc = {};
    lightCbvDesc.BufferLocation = m_lightCB->GetGPUVirtualAddress();
    lightCbvDesc.SizeInBytes = lightCBSize;
    m_device->CreateConstantBufferView(&lightCbvDesc, m_sharedDescriptorAllocator->GetCPUHandle(kLightCBVIndex));

    return true;
} // InitCommonCBs

void Renderer::UpdateCommonCBs() {
    m_Camera->Update();
    m_DirectionalLight->Update();

    // Frame 데이터 채우기
    SharedStructs::FrameCB frameData;
    frameData.view = XMMatrixTranspose(m_Camera->GetViewMatrix());
    frameData.projection = XMMatrixTranspose(m_Camera->GetProjectionMatrix());
    frameData.cameraPosition = m_Camera->GetPosition();
    frameData.cameraFov = m_Camera->GetFov();

    // 메모리 복사
    memcpy(m_mappedFrameCB, &frameData, sizeof(SharedStructs::FrameCB));

    // Light 데이터 채우기
    SharedStructs::DirectionalLightCB lightData;
    lightData.direction = m_DirectionalLight->GetDirection();
    lightData.ambient = m_DirectionalLight->GetAmbient();
    lightData.diffuse = m_DirectionalLight->GetDiffuse();
    lightData.lookAt = m_DirectionalLight->GetLookAt();
    lightData.lightViewMatrix = XMMatrixTranspose(m_DirectionalLight->GetViewMatrix());
    lightData.lightProjectionMatrix = XMMatrixTranspose(m_DirectionalLight->GetProjection());

    // 메모리 복사
    memcpy(m_mappedLightCB, &lightData, sizeof(SharedStructs::DirectionalLightCB));
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
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()); // PSO 없으면 nullptr로 리셋됨

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // CBV/텍스처가 함께 들어있는 공유 힙을 바인딩 (프레임당 한 번)
    ID3D12DescriptorHeap* heaps[] = { m_sharedDescriptorAllocator->GetHeap() };
    m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 테이블 0: CBV (FrameCB, LightCB) - 힙의 0번 인덱스부터 시작하는 테이블
    m_commandList->SetGraphicsRootDescriptorTable(0, m_sharedDescriptorAllocator->GetGPUHandle(kFrameCBVIndex));
    // 테이블 1: 텍스처 SRV - Sponza 머티리얼이 들어오기 전까진 실질적으로 사용되지 않음
    m_commandList->SetGraphicsRootDescriptorTable(1, m_sharedDescriptorAllocator->GetGPUHandle(kReservedDescriptorCount));

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // 베리어: 프리젠트 상태 -> 렌더 타겟 상태로 전환
    D3D12_RESOURCE_BARRIER barrierRT = {};
    barrierRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierRT.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrierRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrierRT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 배경색 지우기 - Sponza 지오메트리가 들어오기 전까진 이 화면이 전부임
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // TODO(Sponza): 여기에 메시별 IASetVertexBuffers/IASetIndexBuffer + DrawIndexedInstanced 추가 예정

    m_ImGuiManager->RenderDrawData(m_commandList.Get());

    // 베리어: 렌더 타겟 상태 -> 프리젠트 상태로 복구
    D3D12_RESOURCE_BARRIER barrierPresent = {};
    barrierPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrierPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrierPresent);

    m_commandList->Close();
} // PopulateCommandList

bool Renderer::LoadGUIs(HWND hwnd, std::shared_ptr<ImGuiManager> imGuiManager) {
    m_ImGuiManager = imGuiManager;
    ImGuiManager::InitParams guiParams;
    guiParams.hwnd = hwnd;
    guiParams.device = m_device.Get();
    guiParams.numFramesInFlight = FrameCount;
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

void Renderer::WaitForPreviousFrame() {
    const UINT64 fence = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fence);
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
} // WaitForPreviousFrame

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

} // OnGUI