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
    m_width(0), m_height(0), m_mipLevels(1), m_sharedDescriptorAllocator(nullptr), m_arraySize(1) {
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
    m_arraySize = params.arraySize > 0 ? params.arraySize : 1;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = static_cast<UINT16>(m_arraySize);
    desc.MipLevels = params.mipLevels > 0 ? params.mipLevels : 1;
    desc.SampleDesc.Count = 1;

    m_mipLevels = desc.MipLevels;
    D3D12_CLEAR_VALUE clearValue = {};

    if (m_type == RenderTextureType::Depth) {
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = params.depthClearValue;
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

        if (m_arraySize > 1) {
            // ----------------------------------------
            // 배열 Depth - 슬라이스별 DSV 생성
            // ----------------------------------------
            m_dsvSliceHandles.resize(m_arraySize);
            for (UINT slice = 0; slice < m_arraySize; ++slice) {
                D3D12_DEPTH_STENCIL_VIEW_DESC sliceDsvDesc = {};
                sliceDsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
                sliceDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                sliceDsvDesc.Texture2DArray.MipSlice = 0;
                sliceDsvDesc.Texture2DArray.FirstArraySlice = slice;
                sliceDsvDesc.Texture2DArray.ArraySize = 1;

                UINT dsvIndex = params.dsvAllocator->Allocate();
                m_dsvSliceHandles[slice] = params.dsvAllocator->GetCPUHandle(dsvIndex);
                params.device->CreateDepthStencilView(m_resource.Get(), &sliceDsvDesc, m_dsvSliceHandles[slice]);
            }
            // 단일 핸들 접근(GetDSVHandle())도 깨지지 않도록 0번 슬라이스로 채워둠
            m_dsvHandle = m_dsvSliceHandles[0];

            // SRV: 전체 슬라이스를 한 번에 바인딩하는 Texture2DArray
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = m_mipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = m_arraySize;

            m_srvIndex = params.sharedDescriptorAllocator->Allocate();
            params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));

            m_mipSrvIndices.resize(m_arraySize);
            for (UINT slice = 0; slice < m_arraySize; ++slice) {
                D3D12_SHADER_RESOURCE_VIEW_DESC sliceSrvDesc = {};
                sliceSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                sliceSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                sliceSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                sliceSrvDesc.Texture2DArray.MipLevels = m_mipLevels;
                sliceSrvDesc.Texture2DArray.FirstArraySlice = slice;
                sliceSrvDesc.Texture2DArray.ArraySize = 1;
                m_mipSrvIndices[slice] = params.sharedDescriptorAllocator->Allocate();
                params.device->CreateShaderResourceView(m_resource.Get(), &sliceSrvDesc,
                    params.sharedDescriptorAllocator->GetCPUHandle(m_mipSrvIndices[slice]));
            }
        }
        else {
            // ----------------------------------------
            // 단일 Depth
            // ----------------------------------------
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
    }
    else {
        if (!params.rtvAllocator) {
            DebugPrint("RenderTexture::Init - Normal/UAV 타입인데 rtvAllocator가 없음");
            return false;
        }

        if (m_arraySize > 1) {
            // ----------------------------------------
            // 배열 RTV/UAV
            // ----------------------------------------
            m_rtvSliceHandles.resize(m_arraySize);
            for (UINT slice = 0; slice < m_arraySize; ++slice) {
                D3D12_RENDER_TARGET_VIEW_DESC sliceRtvDesc = {};
                sliceRtvDesc.Format = params.format;
                sliceRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                sliceRtvDesc.Texture2DArray.MipSlice = 0;
                sliceRtvDesc.Texture2DArray.FirstArraySlice = slice;
                sliceRtvDesc.Texture2DArray.ArraySize = 1;

                UINT rtvIndex = params.rtvAllocator->Allocate();
                m_rtvSliceHandles[slice] = params.rtvAllocator->GetCPUHandle(rtvIndex);
                params.device->CreateRenderTargetView(m_resource.Get(), &sliceRtvDesc, m_rtvSliceHandles[slice]);
            }
            m_rtvHandle = m_rtvSliceHandles[0];

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = params.format;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = m_mipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = m_arraySize;

            m_srvIndex = params.sharedDescriptorAllocator->Allocate();
            params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));
        }
        else {
            // ----------------------------------------
            // 단일 RTV/UAV
            // ----------------------------------------
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
                    D3D12_SHADER_RESOURCE_VIEW_DESC mipSrvDesc = srvDesc;
                    mipSrvDesc.Texture2D.MostDetailedMip = i;
                    mipSrvDesc.Texture2D.MipLevels = 1;

                    m_mipSrvIndices[i] = params.sharedDescriptorAllocator->Allocate();
                    params.device->CreateShaderResourceView(
                        m_resource.Get(), &mipSrvDesc,
                        params.sharedDescriptorAllocator->GetCPUHandle(m_mipSrvIndices[i]));

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
        } // if - else (m_arraySize)
    } // if - else (Depth / Normal-UAV)

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
UINT                             RenderTexture::GetSRVIndex(UINT slice) const {
    return slice < m_mipSrvIndices.size() ? m_mipSrvIndices[slice] : UINT_MAX;
} // GetSRVIndex
UINT                             RenderTexture::GetUAVIndex() const { return m_uavIndex; } // GetUAVIndex
UINT                             RenderTexture::GetWidth() const { return m_width; } // GetWidth
UINT                             RenderTexture::GetHeight() const { return m_height; } // GetHeight
UINT                             RenderTexture::GetMipLevels() const { return m_mipLevels; } // GetMipLevels
UINT                             RenderTexture::GetArraySize() const { return m_arraySize; } // GetArraySize

D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::GetMipSRVGPUHandle(UINT mip) const {
    if (!m_sharedDescriptorAllocator || mip >= m_mipSrvIndices.size()) return { 0 };
    return m_sharedDescriptorAllocator->GetGPUHandle(m_mipSrvIndices[mip]);
} // GetMipSRVGPUHandle

D3D12_GPU_DESCRIPTOR_HANDLE RenderTexture::GetMipUAVGPUHandle(UINT mip) const {
    if (!m_sharedDescriptorAllocator || mip >= m_mipUavIndices.size()) return { 0 };
    return m_sharedDescriptorAllocator->GetGPUHandle(m_mipUavIndices[mip]);
} // GetMipUAVGPUHandle

D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetDSVHandle(UINT slice) const {
    if (slice < m_dsvSliceHandles.size()) {
        return m_dsvSliceHandles[slice];
    }
    return m_dsvHandle;
} // GetDSVHandle