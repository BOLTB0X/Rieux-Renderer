#include "Pch.h"
#include "HierarchicalZBuffer.h"
// Core
#include "RendererState.h"
// D3D12
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "RenderTexture.h"
#include "d3d12.h"
// Utils
#include "ShaderHelper.h"
#include "DebugHelper.h"
// STL
#include <algorithm>

HierarchicalZBuffer::HierarchicalZBuffer()
	: m_device(nullptr), m_rootSignature(nullptr), m_pso(nullptr), m_hasValidHiz(false) {
} // HierarchicalZBuffer

HierarchicalZBuffer::~HierarchicalZBuffer() {
	m_device = nullptr;
	m_rootSignature = nullptr;
	m_pso = nullptr;
} // ~HierarchicalZBuffer

bool HierarchicalZBuffer::Init(const InitParams& params) {
    if (!params.device) {
        DebugHelper::DebugPrint("HierarchicalZBuffer 초기화 실패");
        return false;
    }

    m_device = params.device;
	m_rootSignature = params.rootSignature;
	m_pso = params.pso;

    return true;
} // Init

void HierarchicalZBuffer::Build(const BuildParams& params) {
    if (!params.cmdList || !params.depthTexture || !params.hizTexture) {
        return;
    }

    auto cmdList = params.cmdList;
    auto depthTex = params.depthTexture;
    auto hizTex = params.hizTexture;

    ID3D12Resource* depthResource = depthTex->GetResource();
    ID3D12Resource* hizResource = hizTex->GetResource();

    uint32_t mipLevels = hizTex->GetMipLevels();
    uint32_t width = hizTex->GetWidth();
    uint32_t height = hizTex->GetHeight();

    if (mipLevels <= 1) {
        return;
    }

    // 원본 Depth 복사를 위한 리소스 상태 전환
    depthTex->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    hizTex->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    // 원본 Depth -> Hi-Z Mip 0으로 복사
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = hizResource;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0; // Hi-Z의 밉 0

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = depthResource;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0; // 원본 뎁스는 밉이 1개뿐이니 0

    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // 복사 완료 후 상태 전환
    // 원본 Depth는 다시 뎁스 읽기 상태로 복구
    depthTex->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_READ);

    // Hi-Z 텍스처는 현재 전체가 COPY_DEST 상태이므로
    // Mip별로 올바른 목적지 상태로 전환
    std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
    barriers.reserve(mipLevels);

    // Mip 0:SRV으로 쓰기 위해 NON_PIXEL_SHADER_RESOURCE로 전환
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        hizResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        0
    ));

    // Mip 1 ~ N-1: UAV으로 쓰기 위해 UNORDERED_ACCESS로 전환
    for (uint32_t i = 1; i < mipLevels; ++i) {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            hizResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            i
        ));
    }

    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // ---------------------------------------------------------------------------------
    // Hi-Z 밉맵 체인 빌드
    // ---------------------------------------------------------------------------------
    cmdList->SetPipelineState(m_pso);
    cmdList->SetComputeRootSignature(m_rootSignature);

    // Downsample 루프 (Mip 0 -> 1, Mip 1 -> 2 ...)
    for (uint32_t i = 1; i < mipLevels; ++i) {
        uint32_t inputWidth = std::max(1u, width >> (i - 1));
        uint32_t inputHeight = std::max(1u, height >> (i - 1));
        uint32_t outputWidth = std::max(1u, width >> i);
        uint32_t outputHeight = std::max(1u, height >> i);

        // 상수 버퍼 세팅
        uint32_t cbData[2] = { inputWidth, inputHeight };
        cmdList->SetComputeRoot32BitConstants(RendererState::HZBConstantsIndex, 2, cbData, 0);

        // 디스크립터 세팅
        cmdList->SetComputeRootDescriptorTable(RendererState::DepthTextureIndex, hizTex->GetMipSRVGPUHandle(i - 1));
        cmdList->SetComputeRootDescriptorTable(RendererState::HZBTextureIndex, hizTex->GetMipUAVGPUHandle(i));

        // Dispatch
        uint32_t dispatchX = (outputWidth + 7) / 8;
        uint32_t dispatchY = (outputHeight + 7) / 8;
        cmdList->Dispatch(dispatchX, dispatchY, 1);

        if (i < mipLevels - 1) {
            CD3DX12_RESOURCE_BARRIER toSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
                hizResource,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                i
            );
            cmdList->ResourceBarrier(1, &toSrvBarrier);
        }
    } // for (uint32_t i = 1; i < mipLevels; ++i)
    
    // 마지막 밉 SRV 상태 복구
    CD3DX12_RESOURCE_BARRIER finalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        hizResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        mipLevels - 1
    );
    cmdList->ResourceBarrier(1, &finalBarrier);
    hizTex->SetCurrentStateWithoutBarrier(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
} // Build

void HierarchicalZBuffer::OnHiZ() {
    m_hasValidHiz = true;
} // HiZOn

bool HierarchicalZBuffer::IshasValidHiz() {
    return m_hasValidHiz;
} // IshasValidHiz