#include "Pch.h"
#include "Renderer.h"
// Graphics
#include "Components/Camera.h"
#include "Components/DirectionalLight.h"
#include "Components/GPUMonitor.h"
#include "Components/ImGuiManager.h"
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

Renderer::Renderer() : m_viewport{}, m_scissorRect{}, m_rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM), m_vertexBufferView{},
                       m_frameIndex(0), m_rtvDescriptorSize(0), m_fenceValue(0), m_fenceEvent(nullptr) {
    m_Camera = std::make_unique<Camera>();
    m_DirectionalLight = std::make_unique<DirectionalLight>();
    m_GPUMonitor = std::make_unique<GPUMonitor>();
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
    if (!LoadAssets()) {
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

    if (!InitCommonCBs()) {
        DebugHelper::DebugPrint("InitCommonCBs() failed.");
        return false;
    }

    if (!m_GPUMonitor->Init(m_iDXGIAdapter3.Get())) {
        return false;
    }

    return true;
} // LoadPipeline

bool Renderer::LoadAssets() {
    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // 매개변수 0: FrameCB (b0)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0 레지스터
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // 매개변수 1: DirectionalLightCB (b1)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1; // b1 레지스터
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters); // 2개 파라미터 등록
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

    // 셰이더 컴파일
    ComPtr<IDxcBlob> vertexShader;
    ComPtr<IDxcBlob> pixelShader;
    if (!ShaderHelper::InitVertexShader(L"HLSL/HelloTriangleVS.hlsl", &vertexShader) ||
        !ShaderHelper::InitPixelShader(L"HLSL/HelloTrianglePS.hlsl", &pixelShader)) {
        return false;
    }

    // 인풋 레이아웃
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO (파이프라인 상태 객체) 생성
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_rtvFormat;
    psoDesc.SampleDesc.Count = 1;
    m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

    // 커맨드 리스트 생성
    m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList));
    m_commandList->Close(); // 첫 할당 시 닫아두어야 함

    Vertex triangleVertices[] = {
        { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };
    const UINT vertexBufferSize = sizeof(triangleVertices);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = vertexBufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

    // GPU 메모리에 데이터 복사
    UINT8* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    // 동기화 객체 생성
    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return true;
} // LoadAssets

bool Renderer::InitCommonCBs() {
    m_Camera->Init(SharedConstants::DEFAULT_FOV,
        (float)SharedConstants::SCREEN_WIDTH / (float)SharedConstants::SCREEN_HEIGHT,
        SharedConstants::SCREEN_NEAR, SharedConstants::SCREEN_DEPTH);
    m_DirectionalLight->Init(); 

    // 업로드 힙 속성 준비
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    CD3DX12_RESOURCE_DESC frameCBDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SharedStructs::FrameCB));
    m_device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &frameCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_frameCB));
    m_frameCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedFrameCB));

    CD3DX12_RESOURCE_DESC lightCBDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SharedStructs::DirectionalLightCB));
    m_device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &lightCBDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_lightCB));
    m_lightCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedLightCB));

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
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    m_commandList->SetGraphicsRootConstantBufferView(0, m_frameCB->GetGPUVirtualAddress()); // b0
    m_commandList->SetGraphicsRootConstantBufferView(1, m_lightCB->GetGPUVirtualAddress()); // b1

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

    // 배경색 지우기
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

    // 드로 콜!
    m_commandList->DrawInstanced(3, 1, 0, 0);

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