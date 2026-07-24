#include "Pch.h"
#include "RenderTarget.h"
// Components
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

RenderTarget::RenderTarget()
    : m_rtvHandle{}, m_dsvHandle{}, m_colorSRVIndex(UINT_MAX), m_viewport{}, m_scissorRect{} {
} // RenderTarget

RenderTarget::~RenderTarget() {
} // ~RenderTarget

bool RenderTarget::Init(const InitParams& params) {
    if (!params.device || !params.sharedDescriptorAllocator || params.width == 0 || params.height == 0) {
        DebugPrint("RenderTarget::Init - 잘못된 파라미터");
        return false;
    }

    m_viewport = { 0.0f, 0.0f, static_cast<float>(params.width), static_cast<float>(params.height), 0.0f, 1.0f };
    m_scissorRect = { 0, 0, static_cast<LONG>(params.width), static_cast<LONG>(params.height) };

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    // --------------------------------------------------
    // 컬러 텍스처
    // --------------------------------------------------
    D3D12_RESOURCE_DESC colorDesc = {};
    colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    colorDesc.Width = params.width;
    colorDesc.Height = params.height;
    colorDesc.DepthOrArraySize = 1;
    colorDesc.MipLevels = 1;
    colorDesc.Format = params.colorFormat;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClear = {};
    colorClear.Format = params.colorFormat;
    colorClear.Color[0] = 0.0f; colorClear.Color[1] = 0.2f; colorClear.Color[2] = 0.4f; colorClear.Color[3] = 1.0f;

    if (FAILED(params.device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &colorDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &colorClear, IID_PPV_ARGS(&m_colorTexture)))) {
        DebugPrint("RenderTarget 컬러 텍스처 생성 실패");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    if (FAILED(params.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
        DebugPrint("RenderTarget RTV 힙 생성 실패");
        return false;
    }
    m_rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    params.device->CreateRenderTargetView(m_colorTexture.Get(), nullptr, m_rtvHandle);

    // 공유 힙에 컬러 SRV 등록
    m_colorSRVIndex = params.sharedDescriptorAllocator->Allocate();
    if (m_colorSRVIndex == UINT_MAX) {
        DebugPrint("RenderTarget 컬러 SRV 디스크립터 할당 실패");
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = params.colorFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    params.device->CreateShaderResourceView(
        m_colorTexture.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_colorSRVIndex));

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = params.width;
    depthDesc.Height = params.height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = params.depthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClear = {};
    depthClear.Format = params.depthFormat;
    depthClear.DepthStencil.Depth = 1.0f;
    depthClear.DepthStencil.Stencil = 0;

    if (FAILED(params.device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&m_depthTexture)))) {
        DebugPrint("RenderTarget 뎁스 텍스처 생성 실패");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    if (FAILED(params.device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) {
        DebugPrint("RenderTarget DSV 힙 생성 실패");
        return false;
    }
    m_dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    params.device->CreateDepthStencilView(m_depthTexture.Get(), nullptr, m_dsvHandle);

    return true;
} // Init

void RenderTarget::BeginRender(ID3D12GraphicsCommandList* cmdList, const float clearColor[4]) {
    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
        m_colorTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &toRT);

    cmdList->OMSetRenderTargets(1, &m_rtvHandle, FALSE, &m_dsvHandle);
    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);

    cmdList->ClearRenderTargetView(m_rtvHandle, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
} // BeginRender

void RenderTarget::EndRender(ID3D12GraphicsCommandList* cmdList) {
    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        m_colorTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &toSRV);
} // EndRender

UINT RenderTarget::GetColorSRVIndex() const {
    return m_colorSRVIndex;
} // GetColorSRVIndex

ID3D12Resource* RenderTarget::GetColorResource() const {
    return m_colorTexture.Get();
} // GetColorResource

const D3D12_VIEWPORT& RenderTarget::GetViewport() const {
    return m_viewport;
} // GetViewport

const D3D12_RECT& RenderTarget::GetScissorRect() const {
    return m_scissorRect;
} // GetScissorRect