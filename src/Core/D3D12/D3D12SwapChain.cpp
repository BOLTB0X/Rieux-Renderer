#include "Pch.h"
#include "D3D12SwapChain.h"
// Core
#include "RendererState.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;
using namespace DebugHelper;

D3D12SwapChain::D3D12SwapChain()
    : m_viewport{}, m_scissorRect{},
      m_rtvDescriptorSize(0), m_frameIndex(0), m_frameCount(0) {
} // D3D12SwapChain

D3D12SwapChain::~D3D12SwapChain() {
} // ~D3D12SwapChain

bool D3D12SwapChain::Init(const InitParams& params) {
    if (!params.hwnd || !params.device || !params.commandQueue || params.width == 0 || params.height == 0) {
        DebugPrint("D3D12SwapChain::Init - 잘못된 파라미터");
        return false;
    }

    m_frameCount = params.frameCount;

    m_viewport = { 0.0f, 0.0f, static_cast<float>(RendererState::ScreenWidth), static_cast<float>(RendererState::ScreenHeight), 0.0f, 1.0f };
    m_scissorRect = { 0, 0,  RendererState::ScreenWidth, RendererState::ScreenHeight };

    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        DebugPrint("DXGI 팩토리 생성 실패");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_frameCount;
    swapChainDesc.Width = params.width;
    swapChainDesc.Height = params.height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(params.commandQueue, params.hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1))) {
        DebugPrint("스왑체인 생성 실패");
        return false;
    }

    if (FAILED(swapChain1.As(&m_swapChain))) {
        DebugPrint("IDXGISwapChain3 변환 실패");
        return false;
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(params.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        DebugPrint("백버퍼용 RTV 힙 생성 실패");
        return false;
    }
    m_rtvDescriptorSize = params.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_backBuffers.resize(m_frameCount);
    m_rtvHandles.resize(m_frameCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < m_frameCount; ++i) {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])))) {
            DebugPrint("백버퍼 가져오기 실패");
            return false;
        }
        params.device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        m_rtvHandles[i] = rtvHandle; // 프레임별로 한 번만 계산해서 캐싱
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    return true;
} // Init

void D3D12SwapChain::Present(bool vsync) {
    m_swapChain->Present(vsync ? 1 : 0, 0);
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
} // Present

const D3D12_VIEWPORT& D3D12SwapChain::GetViewport() const {
    return m_viewport;
} // GetViewport

const D3D12_RECT& D3D12SwapChain::GetScissorRect() const {
    return m_scissorRect;
} // GetScissorRect

ID3D12Resource* D3D12SwapChain::GetCurrentBackBuffer() const {
    return m_backBuffers[m_frameIndex].Get();
} // GetCurrentBackBuffer

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentRTVHandle() const {
    return m_rtvHandles[m_frameIndex];
} // GetCurrentRTVHandle

UINT D3D12SwapChain::GetCurrentFrameIndex() const {
    return m_frameIndex;
} // GetCurrentFrameIndex