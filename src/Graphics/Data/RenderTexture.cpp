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
    m_width(0), m_height(0), m_mipLevels(1), m_sharedDescriptorAllocator(nullptr),
    m_arraySize(1), m_isCubeMap(false) {
} // RenderTexture

RenderTexture::~RenderTexture() {
    m_sharedDescriptorAllocator = nullptr;
} // ~RenderTexture

bool RenderTexture::Init(const InitParams& params) {
    // 기본 파라미터 검증 및 멤버 변수 초기화
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
    m_mipLevels = params.mipLevels > 0 ? params.mipLevels : 1;
    m_isCubeMap = params.isCubeMap;

    if (!CreateTextureResource(params)) {
        return false;
    }

    // 타입에 따른 View(Descriptor) 생성
    if (m_type == RenderTextureType::Depth) {
        if (!params.dsvAllocator) {
            DebugPrint("RenderTexture::Init - Depth 타입인데 dsvAllocator가 없음");
            return false;
        }
        if (!CreateDepthViews(params)) return false;
    }
    else { // Normal 또는 UAV
        if (!params.rtvAllocator) {
            DebugPrint("RenderTexture::Init - Normal/UAV 타입인데 rtvAllocator가 없음");
            return false;
        }
        if (!CreateRenderTargetViews(params)) return false;

        if (m_type == RenderTextureType::UAV) {
            if (!CreateUnorderedAccessViews(params)) return false;
        }
    }

    if (!CreateShaderResourceViews(params)) {
        return false;
    }

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
UINT                             RenderTexture::GetSRVIndex() const { return m_srvIndex; } // GetSliceSRVIndex
UINT                             RenderTexture::GetSliceSRVIndex(UINT slice) const { return slice < m_sliceSrvIndices.size() ? m_sliceSrvIndices[slice] : UINT_MAX; } // GetSliceSRVIndex
UINT                             RenderTexture::GetUAVIndex() const { return m_uavIndex; } // GetUAVIndex
UINT                             RenderTexture::GetWidth() const { return m_width; } // GetWidth
UINT                             RenderTexture::GetHeight() const { return m_height; } // GetHeight
UINT                             RenderTexture::GetMipLevels() const { return m_mipLevels; } // GetMipLevels
UINT                             RenderTexture::GetArraySize() const { return m_arraySize; } // GetArraySize
bool                             RenderTexture::IsCubeMap() const { return m_isCubeMap; }

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

D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetRTVHandle(UINT slice) const {
    if (slice < m_rtvSliceHandles.size()) {
        return m_rtvSliceHandles[slice];
    }
    return m_rtvHandle;
} // GetRTVHandle

UINT RenderTexture::GetMipFaceUAVIndex(UINT mip, UINT face) const {
    UINT idx = mip * m_arraySize + face;
    return idx < m_mipFaceUavIndices.size() ? m_mipFaceUavIndices[idx] : UINT_MAX;
} // GetMipFaceUAVIndex

UINT RenderTexture::GetMipFaceSRVIndex(UINT mip, UINT face) const {
    UINT idx = mip * m_arraySize + face;
    return idx < m_mipFaceSrvIndices.size() ? m_mipFaceSrvIndices[idx] : UINT_MAX;
}

bool RenderTexture::CreateTextureResource(const InitParams& params) {
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = static_cast<UINT16>(m_arraySize);
    desc.MipLevels = m_mipLevels;
    desc.SampleDesc.Count = 1;

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
    return true;
} // CreateTextureResource

bool RenderTexture::CreateDepthViews(const InitParams& params) {
    if (m_arraySize > 1) {
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
        m_dsvHandle = m_dsvSliceHandles[0];
    }
    else {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        UINT dsvIndex = params.dsvAllocator->Allocate();
        m_dsvHandle = params.dsvAllocator->GetCPUHandle(dsvIndex);
        params.device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsvHandle);
    }
    return true;
} // CreateDepthViews

bool RenderTexture::CreateRenderTargetViews(const InitParams& params) {
    if (m_arraySize > 1) {
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
    }
    else {
        UINT rtvIndex = params.rtvAllocator->Allocate();
        m_rtvHandle = params.rtvAllocator->GetCPUHandle(rtvIndex);
        params.device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtvHandle);
    }
    return true;
} // CreateRenderTargetViews

bool RenderTexture::CreateShaderResourceViews(const InitParams& params) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = (m_type == RenderTextureType::Depth) ? DXGI_FORMAT_R32_FLOAT : params.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (m_arraySize > 1) {
        if (m_isCubeMap && m_type != RenderTextureType::Depth) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.MipLevels = m_mipLevels;
            srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = m_mipLevels;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = m_arraySize;
        }

        m_srvIndex = params.sharedDescriptorAllocator->Allocate();
        params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));

        // 슬라이스별 단일 SRV
        m_sliceSrvIndices.resize(m_arraySize);
        for (UINT slice = 0; slice < m_arraySize; ++slice) {
            D3D12_SHADER_RESOURCE_VIEW_DESC sliceSrvDesc = {};
            sliceSrvDesc.Format = srvDesc.Format;
            sliceSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            sliceSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            sliceSrvDesc.Texture2DArray.MostDetailedMip = 0;
            sliceSrvDesc.Texture2DArray.MipLevels = m_mipLevels;
            sliceSrvDesc.Texture2DArray.FirstArraySlice = slice;
            sliceSrvDesc.Texture2DArray.ArraySize = 1;

            m_sliceSrvIndices[slice] = params.sharedDescriptorAllocator->Allocate();
            params.device->CreateShaderResourceView(m_resource.Get(), &sliceSrvDesc,
                params.sharedDescriptorAllocator->GetCPUHandle(m_sliceSrvIndices[slice]));
        }
    }
    else {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = m_mipLevels;

        m_srvIndex = params.sharedDescriptorAllocator->Allocate();
        params.device->CreateShaderResourceView(m_resource.Get(), &srvDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_srvIndex));
    }
    return true;
} // CreateShaderResourceViews

bool RenderTexture::CreateUnorderedAccessViews(const InitParams& params) {
    if (m_arraySize > 1) {
        if (m_isCubeMap && m_mipLevels > 1) {
            m_mipFaceUavIndices.resize(m_mipLevels * m_arraySize);
            m_mipFaceSrvIndices.resize(m_mipLevels * m_arraySize);

            for (UINT mip = 0; mip < m_mipLevels; ++mip) {
                for (UINT face = 0; face < m_arraySize; ++face) {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC mipFaceUav = {};
                    mipFaceUav.Format = params.format;
                    mipFaceUav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    mipFaceUav.Texture2DArray.MipSlice = mip;
                    mipFaceUav.Texture2DArray.FirstArraySlice = face;
                    mipFaceUav.Texture2DArray.ArraySize = 1;

                    UINT idx = params.sharedDescriptorAllocator->Allocate();
                    params.device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &mipFaceUav,
                        params.sharedDescriptorAllocator->GetCPUHandle(idx));
                    m_mipFaceUavIndices[mip * m_arraySize + face] = idx;

                    // 디버깅용
                    D3D12_SHADER_RESOURCE_VIEW_DESC mipFaceSrv = {};
                    mipFaceSrv.Format = params.format;
                    mipFaceSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    mipFaceSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    mipFaceSrv.Texture2DArray.MostDetailedMip = mip;
                    mipFaceSrv.Texture2DArray.MipLevels = 1;
                    mipFaceSrv.Texture2DArray.FirstArraySlice = face;
                    mipFaceSrv.Texture2DArray.ArraySize = 1;

                    UINT srvIdx = params.sharedDescriptorAllocator->Allocate();
                    params.device->CreateShaderResourceView(m_resource.Get(), &mipFaceSrv,
                        params.sharedDescriptorAllocator->GetCPUHandle(srvIdx));
                    m_mipFaceSrvIndices[mip * m_arraySize + face] = srvIdx;
                } // for (UINT face = 0; face < m_arraySize; ++face)
            } // for (UINT mip = 0; mip < m_mipLevels; ++mip)
        } // if (m_isCubeMap && m_mipLevels > 1)
    }
    else {
        if (m_mipLevels <= 1) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = params.format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

            m_uavIndex = params.sharedDescriptorAllocator->Allocate();
            params.device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, params.sharedDescriptorAllocator->GetCPUHandle(m_uavIndex));
        }
        else {
            m_mipSrvIndices.resize(m_mipLevels);
            m_mipUavIndices.resize(m_mipLevels);

            for (UINT i = 0; i < m_mipLevels; ++i) {
                // 단일 텍스처의 밉 레벨별 SRV
                D3D12_SHADER_RESOURCE_VIEW_DESC mipSrvDesc = {};
                mipSrvDesc.Format = params.format;
                mipSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                mipSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                mipSrvDesc.Texture2D.MostDetailedMip = i;
                mipSrvDesc.Texture2D.MipLevels = 1;

                m_mipSrvIndices[i] = params.sharedDescriptorAllocator->Allocate();
                params.device->CreateShaderResourceView(
                    m_resource.Get(), &mipSrvDesc,
                    params.sharedDescriptorAllocator->GetCPUHandle(m_mipSrvIndices[i]));

                // 단일 텍스처의 밉 레벨별 UAV
                D3D12_UNORDERED_ACCESS_VIEW_DESC mipUavDesc = {};
                mipUavDesc.Format = params.format;
                mipUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                mipUavDesc.Texture2D.MipSlice = i;

                m_mipUavIndices[i] = params.sharedDescriptorAllocator->Allocate();
                params.device->CreateUnorderedAccessView(
                    m_resource.Get(), nullptr, &mipUavDesc,
                    params.sharedDescriptorAllocator->GetCPUHandle(m_mipUavIndices[i]));
            }
        }
    }
    return true;
} // CreateUnorderedAccessViews