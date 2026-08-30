#include "Pch.h"
#include "BRDFIntegrationLUT.h"
// Core
#include "RendererState.h"
// Components
#include "RenderTextureManager.h"
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"
#include "SharedCommons.h"

using namespace DebugHelper;

namespace {
    const UINT BRDF_LUT_SIZE = 512;
}

BRDFIntegrationLUT::BRDFIntegrationLUT()
    : m_rootSignature(nullptr), m_computePSO(nullptr), m_lutTexture(nullptr),
    m_mappedConstantBuffer(nullptr), m_sharedDescriptorAllocator(nullptr),
    m_lutSize(0), m_format(DXGI_FORMAT_R16G16_FLOAT), m_isGenerated(false) {
} // BRDFIntegrationLUT

BRDFIntegrationLUT::~BRDFIntegrationLUT() {
    if (m_constantBuffer && m_mappedConstantBuffer) {
        m_constantBuffer->Unmap(0, nullptr);
    }
    m_mappedConstantBuffer = nullptr;
    m_lutTexture = nullptr;
    m_sharedDescriptorAllocator = nullptr;
} // BRDFIntegrationLUT

bool BRDFIntegrationLUT::Init(const InitParams& params) {
    if (!params.device || !params.rootSignature || !params.computePSO ||
        !params.renderTextureManager || !params.sharedDescriptorAllocator) {
        DebugPrint("BRDFIntegrationLUT::Init - 잘못된 파라미터");
        return false;
    }

    m_rootSignature = params.rootSignature;
    m_computePSO = params.computePSO;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;
    m_lutSize = BRDF_LUT_SIZE;
    m_format = params.format;
    m_isGenerated = false;

    const std::shared_ptr<RenderTexture> texture = params.renderTextureManager->CreateRenderTexture(
            SharedCommons::KEY_BRDF_LUT_RENDER_TEXTURE, m_lutSize, m_lutSize,
            RenderTexture::RenderTextureType::UAV, m_format);
    if (!texture) {
        DebugPrint("BRDFIntegrationLUT 텍스처 생성 실패");
        return false;
    }
    m_lutTexture = texture.get();

    const UINT constantBufferSize = (sizeof(BRDFLUTCB) + 255u) & ~255u;
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC constantBufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
    if (FAILED(params.device->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_constantBuffer)))) {
        DebugPrint("BRDFIntegrationLUT 상수 버퍼 생성 실패");
        m_lutTexture = nullptr;
        return false;
    }

    if (FAILED(m_constantBuffer->Map(
        0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBuffer)))) {
        DebugPrint("BRDFIntegrationLUT 상수 버퍼 매핑 실패");
        m_constantBuffer.Reset();
        m_lutTexture = nullptr;
        m_mappedConstantBuffer = nullptr;
        return false;
    }

    BRDFLUTCB cb = {};
    cb.lutSize = m_lutSize;
    memcpy(m_mappedConstantBuffer, &cb, sizeof(cb));
    return true;
} // Init

void BRDFIntegrationLUT::Generate(ID3D12GraphicsCommandList* commandList) {
    if (!commandList || !m_lutTexture || !m_constantBuffer ||
        !m_sharedDescriptorAllocator || m_isGenerated) {
        return;
    }

    m_lutTexture->Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->SetComputeRootSignature(m_rootSignature);
    commandList->SetPipelineState(m_computePSO);
    commandList->SetComputeRootConstantBufferView(
        RendererState::BRDFLUTConstantBufferIndex,
        m_constantBuffer->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(
        RendererState::BRDFLUTIndex,
        m_sharedDescriptorAllocator->GetGPUHandle(m_lutTexture->GetUAVIndex()));

    commandList->Dispatch((m_lutSize + 7) / 8, (m_lutSize + 7) / 8, 1);
    m_lutTexture->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_isGenerated = true;
} // Generate

RenderTexture* BRDFIntegrationLUT::GetTexture() const { 
    return m_lutTexture;
} // GetTexture

ID3D12Resource* BRDFIntegrationLUT::GetResource() const {
    return m_lutTexture ? m_lutTexture->GetResource() : nullptr;
} // GetResource

UINT BRDFIntegrationLUT::GetSRVIndex() const {
    return m_lutTexture ? m_lutTexture->GetSRVIndex() : UINT_MAX;
} // GetSRVIndex

UINT BRDFIntegrationLUT::GetLUTSize() const { return m_lutSize; }
bool BRDFIntegrationLUT::IsGenerated() const { return m_isGenerated; }
