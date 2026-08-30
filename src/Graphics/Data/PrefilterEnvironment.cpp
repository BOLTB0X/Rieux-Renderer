#include "Pch.h"
#include "PrefilterEnvironment.h"
// Core
#include "RendererState.h"
#include "RenderTextureManager.h"
#include "RenderTexture.h"
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

PrefilterEnvironment::PrefilterEnvironment()
    : m_rootSignature(nullptr), m_computePSO(nullptr), m_sourceCubemap(nullptr),
    m_outputTexture(nullptr), m_sharedDescriptorAllocator(nullptr),
    m_mappedConstantBuffer(nullptr), m_outputSize(0), m_mipLevels(0),
    m_sampleCount(0), m_format(DXGI_FORMAT_R16G16B16A16_FLOAT),
    m_isGenerated(false) {
} // PrefilterEnvironment

PrefilterEnvironment::~PrefilterEnvironment() {
    if (m_constantBuffer && m_mappedConstantBuffer) {
        m_constantBuffer->Unmap(0, nullptr);
    }
    m_mappedConstantBuffer = nullptr;
    m_sourceCubemap = nullptr;
    m_outputTexture = nullptr;
    m_sharedDescriptorAllocator = nullptr;
} // PrefilterEnvironment

bool PrefilterEnvironment::Init(const InitParams& params) {
    if (!params.device || !params.rootSignature || !params.computePSO ||
        !params.renderTextureManager || !params.sharedDescriptorAllocator ||
        !params.sourceCubemap || params.outputSize == 0 ||
        params.mipLevels == 0 || params.sampleCount == 0 ||
        params.sourceCubemap->GetArraySize() != 6) {
        DebugPrint("PrefilterEnvironment::Init - 잘못된 파라미터");
        return false;
    }

    m_rootSignature = params.rootSignature;
    m_computePSO = params.computePSO;
    m_sourceCubemap = params.sourceCubemap;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;
    m_outputSize = params.outputSize;
    m_mipLevels = params.mipLevels;
    m_sampleCount = params.sampleCount;
    m_format = params.format;
    m_isGenerated = false;

    auto output = params.renderTextureManager->CreateRenderTexture(
        "PrefilteredEnvironment", m_outputSize, m_outputSize,
        RenderTexture::RenderTextureType::UAV, m_format, m_mipLevels, 6, true);
    if (!output) {
        DebugPrint("PrefilterEnvironment 출력 텍스처 생성 실패");
        return false;
    }
    m_outputTexture = output.get();

    const UINT constantBufferSize = (sizeof(PrefilterCB) + 255u) & ~255u;
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC constantBufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
    if (FAILED(params.device->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_constantBuffer)))) {
        DebugPrint("PrefilterEnvironment 상수 버퍼 생성 실패");
        m_outputTexture = nullptr;
        return false;
    }

    if (FAILED(m_constantBuffer->Map(
        0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBuffer)))) {
        DebugPrint("PrefilterEnvironment 상수 버퍼 매핑 실패");
        m_constantBuffer.Reset();
        m_outputTexture = nullptr;
        m_mappedConstantBuffer = nullptr;
        return false;
    }

    return true;
} // Init

void PrefilterEnvironment::Generate(ID3D12GraphicsCommandList* commandList) {
    if (!commandList || !m_sourceCubemap || !m_outputTexture || m_isGenerated) {
        return;
    }

    m_sourceCubemap->Transition(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_outputTexture->Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetComputeRootSignature(m_rootSignature);
    commandList->SetPipelineState(m_computePSO);
    commandList->SetComputeRootDescriptorTable(
        RendererState::SourceCubemapIndex,
        m_sharedDescriptorAllocator->GetGPUHandle(m_sourceCubemap->GetSRVIndex()));

    for (UINT mip = 0; mip < m_mipLevels; ++mip) {
        const UINT mipSize = (m_outputSize >> mip) > 0 ? (m_outputSize >> mip) : 1;
        const float roughness = m_mipLevels > 1
            ? static_cast<float>(mip) / static_cast<float>(m_mipLevels - 1)
            : 0.0f;
        const UINT mipSamples = m_sampleCount >> mip;
        const UINT sampleCount = mipSamples > 64 ? mipSamples : 64;

        for (UINT face = 0; face < 6; ++face) {
            PrefilterCB cb = {};
            cb.roughness = roughness;
            cb.faceIndex = face;
            cb.outputSize = mipSize;
            cb.sampleCount = sampleCount;

            commandList->SetComputeRoot32BitConstants(
                RendererState::PrefilterConstantBufferIndex, 4, &cb, 0);

            commandList->SetComputeRootDescriptorTable(
                RendererState::OutputMipFaceIndex,
                m_sharedDescriptorAllocator->GetGPUHandle(
                    m_outputTexture->GetMipFaceUAVIndex(mip, face)));
            commandList->Dispatch((mipSize + 7) / 8, (mipSize + 7) / 8, 1);
        }
    }

    m_outputTexture->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_sourceCubemap->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_isGenerated = true;
} // Generate

void PrefilterEnvironment::Invalidate() { m_isGenerated = false; }

RenderTexture* PrefilterEnvironment::GetTexture() const { return m_outputTexture; }

ID3D12Resource* PrefilterEnvironment::GetResource() const {
    return m_outputTexture ? m_outputTexture->GetResource() : nullptr;
} // GetResource

UINT PrefilterEnvironment::GetSRVIndex() const {
    return m_outputTexture ? m_outputTexture->GetSRVIndex() : UINT_MAX;
} // GetResource

UINT PrefilterEnvironment::GetOutputSize() const { return m_outputSize; }
UINT PrefilterEnvironment::GetMipLevels() const { return m_mipLevels; }
bool PrefilterEnvironment::IsGenerated() const { return m_isGenerated; }
