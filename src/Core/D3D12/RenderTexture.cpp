#include "Pch.h"
#include "RenderTexture.h"
// Components
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

RenderTexture::RenderTexture()
    : m_rtvHandle{}, m_dsvHandle{}, m_srvIndex(UINT_MAX), m_uavIndex(UINT_MAX),
    m_currentState(D3D12_RESOURCE_STATE_COMMON), m_type(RenderTextureType::Normal),
    m_width(0), m_height(0), m_mipLevels(1), m_sharedDescriptorAllocator(nullptr) {
} // RenderTexture

RenderTexture::~RenderTexture() {
    m_sharedDescriptorAllocator = nullptr;
} // ~RenderTexture

bool RenderTexture::Init(const InitParams& params) {
    if (!params.device || !params.sharedDescriptorAllocator || params.width == 0 || params.height == 0) {
        DebugPrint("RenderTexture::Init - 잘못된 파라미터");
        return false;
    }

    m_type = params.type;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;
    m_width = params.width;
    m_height = params.height;
    m_currentState = D3D12_RESOURCE_STATE_COMMON;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = params.mipLevels > 0 ? params.mipLevels : 1;
    desc.SampleDesc.Count = 1;

    m_mipLevels = desc.MipLevels;
    D3D12_CLEAR_VALUE clearValue = {};

    if (m_type == RenderTextureType::Depth) {
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 0.0f;
        clearValue.DepthStencil.Stencil = 0;
    }
    else {
        desc.Format = params.format;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (m_type == RenderTextureType::UAV) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        clearValue.Format = params.format;
        clearValue.Color[0] = 0.0f; clearValue.Color[1] = 0.0f; clearValue.Color[2] = 0.0f; clearValue.Color[3] = 1.0f;
    }

    if (FAILED(params.device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &desc,
        m_currentState, &clearValue, IID_PPV_ARGS(&m_resource)))) {
        DebugPrint("RenderTexture 리소스 생성 실패");
        return false;
    }

    if (m_type == RenderTextureType::Depth) {
        if (!params.dsvAllocator) {
            DebugPrint("RenderTexture::Init - Depth 타입인데 dsvAllocator가 없음");
            return false;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        UINT dsvIndex = params.dsvAllocator->Allocate();
        m_dsvHandle = params.dsvAllocator->GetCPUHandle(dsvIndex);
        params.device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = m_mipLevels;

        m_srvIndex = params.sharedDescriptorAllocator->Allocate();
        params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));
    }
    else {
        if (!params.rtvAllocator) {
            DebugPrint("RenderTexture::Init - Normal/UAV 타입인데 rtvAllocator가 없음");
            return false;
        }

        UINT rtvIndex = params.rtvAllocator->Allocate();
        m_rtvHandle = params.rtvAllocator->GetCPUHandle(rtvIndex);
        params.device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtvHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = params.format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = m_mipLevels;

        m_srvIndex = params.sharedDescriptorAllocator->Allocate();
        params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));

        if (m_type == RenderTextureType::UAV && m_mipLevels <= 1) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = params.format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

            m_uavIndex = params.sharedDescriptorAllocator->Allocate();
            params.device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_uavIndex));
        }
        
        else if (m_type == RenderTextureType::UAV && m_mipLevels > 1) {
            m_mipSrvIndices.resize(m_mipLevels);
            m_mipUavIndices.resize(m_mipLevels);

            for (UINT i = 0; i < m_mipLevels; ++i) {
                // 특정 밉 레벨만 읽는 SRV
                D3D12_SHADER_RESOURCE_VIEW_DESC mipSrvDesc = srvDesc;
                mipSrvDesc.Texture2D.MostDetailedMip = i;
                mipSrvDesc.Texture2D.MipLevels = 1; // 딱 1개 레벨

                m_mipSrvIndices[i] = params.sharedDescriptorAllocator->Allocate();
                params.device->CreateShaderResourceView(
                    m_resource.Get(), &mipSrvDesc,
                    params.sharedDescriptorAllocator->GetCPUHandle(m_mipSrvIndices[i]));

                // 특정 밉 레벨에 쓰는 UAV
                D3D12_UNORDERED_ACCESS_VIEW_DESC mipUavDesc = {};
                mipUavDesc.Format = params.format;
                mipUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mipUavDesc.Texture2D.MipSlice = i;

                m_mipUavIndices[i] = params.sharedDescriptorAllocator->Allocate();
                params.device->CreateUnorderedAccessView(
                    m_resource.Get(), nullptr, &mipUavDesc,
                    params.sharedDescriptorAllocator->GetCPUHandle(m_mipUavIndices[i]));
            } // for (UINT i = 0; i < m_mipLevels; ++i)
        }
    } // if - else

    return true;
} // Init

void RenderTexture::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
    if (m_currentState == newState) {
        return;
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), m_currentState, newState);
    cmdList->ResourceBarrier(1, &barrier);
    m_currentState = newState;
} // Transition

void RenderTexture::Clear(ID3D12GraphicsCommandList* cmdList, float r, float g, float b, float a) {
    const float color[4] = { r, g, b, a };
    cmdList->ClearRenderTargetView(m_rtvHandle, color, 0, nullptr);
} // Clear

void RenderTexture::ClearDepth(ID3D12GraphicsCommandList* cmdList, float depth, UINT8 stencil) {
    cmdList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);
} // ClearDepth

void                             RenderTexture::SetCurrentStateWithoutBarrier(D3D12_RESOURCE_STATES state) { m_currentState = state; }
RenderTexture::RenderTextureType RenderTexture::GetType() const { return m_type; }
ID3D12Resource*                  RenderTexture::GetResource() const { return m_resource.Get(); } // GetResource
D3D12_CPU_DESCRIPTOR_HANDLE      RenderTexture::GetRTVHandle() const { return m_rtvHandle; } // GetRTVHandle
D3D12_CPU_DESCRIPTOR_HANDLE      RenderTexture::GetDSVHandle() const { return m_dsvHandle; } // GetDSVHandle
UINT                             RenderTexture::GetSRVIndex() const { return m_srvIndex; } // GetSRVIndex
UINT                             RenderTexture::GetUAVIndex() const { return m_uavIndex; } // GetUAVIndex
UINT                             RenderTexture::GetWidth() const { return m_width; } // GetWidth
UINT                             RenderTexture::GetHeight() const { return m_height; } // GetHeight
UINT                             RenderTexture::GetMipLevels() const { return m_mipLevels; } // GetMipLevels

D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::GetMipSRVGPUHandle(UINT mip) const {
    if (!m_sharedDescriptorAllocator || mip >= m_mipSrvIndices.size()) return { 0 };
    return m_sharedDescriptorAllocator->GetGPUHandle(m_mipSrvIndices[mip]);
} // GetMipSRVGPUHandle

D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::GetMipUAVGPUHandle(UINT mip) const {
    if (!m_sharedDescriptorAllocator || mip >= m_mipUavIndices.size()) return { 0 };
    return m_sharedDescriptorAllocator->GetGPUHandle(m_mipUavIndices[mip]);
} // GetMipUAVGPUHandle