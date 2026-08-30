#include "Pch.h"
#include "IrradianceConvolution.h"
// Core
#include "RendererState.h"
#include "RenderTextureManager.h"
// Data
#include "RenderTexture.h"
// Tools
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

IrradianceConvolution::IrradianceConvolution()
    : m_rootSignature(nullptr), m_computePSO(nullptr), m_sourceCubemap(nullptr),
    m_outputTexture(nullptr), m_sharedDescriptorAllocator(nullptr),
    m_outputSize(32), m_sampleDelta(0.025f), m_isGenerated(false) {
} // IrradianceConvolution

IrradianceConvolution::~IrradianceConvolution() {
} // ~IrradianceConvolution

bool IrradianceConvolution::Init(const InitParams& params) {
    if (!params.device || !params.rootSignature || !params.computePSO ||
        !params.renderTextureManager || !params.sharedDescriptorAllocator ||
        !params.sourceCubemap || params.outputSize == 0) {
        DebugPrint("IrradianceConvolution::Init - 잘못된 파라미터");
        return false;
    }

    m_rootSignature = params.rootSignature;
    m_computePSO = params.computePSO;
    m_sourceCubemap = params.sourceCubemap;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;
    m_outputSize = params.outputSize;
    m_sampleDelta = params.sampleDelta;
    m_isGenerated = false;

    auto output = params.renderTextureManager->CreateRenderTexture(
        "IrradianceMap", m_outputSize, m_outputSize,
        RenderTexture::RenderTextureType::UAV, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 6, true);
    if (!output) {
        DebugPrint("IrradianceConvolution 출력 텍스처 생성 실패");
        return false;
    }
    m_outputTexture = output.get();
    return true;
} // Init

void IrradianceConvolution::Generate(ID3D12GraphicsCommandList* cmdList) {
    if (!cmdList || !m_sourceCubemap || !m_outputTexture || m_isGenerated) {
        return;
    }

    m_sourceCubemap->Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_outputTexture->Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_computePSO);
    cmdList->SetComputeRootDescriptorTable(RendererState::IrradianceSourceCubemapIndex,
        m_sharedDescriptorAllocator->GetGPUHandle(m_sourceCubemap->GetSRVIndex()));

    for (UINT face = 0; face < 6; ++face) {
        IrradianceRootConstants rc = {};
        rc.faceIndex = face;
        rc.outputSize = m_outputSize;
        rc.sampleDelta = m_sampleDelta;

        cmdList->SetComputeRoot32BitConstants(RendererState::IrradianceConstantIndex, 3, &rc, 0);
        cmdList->SetComputeRootDescriptorTable(
            RendererState::IrradianceOutputIndex,
            m_sharedDescriptorAllocator->GetGPUHandle(m_outputTexture->GetMipFaceUAVIndex(0, face)));

        cmdList->Dispatch((m_outputSize + 7) / 8, (m_outputSize + 7) / 8, 1);
    }

    m_outputTexture->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_sourceCubemap->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_isGenerated = true;
} // Generate

void           IrradianceConvolution::Invalidate() { m_isGenerated = false; }
RenderTexture* IrradianceConvolution::GetTexture() const { return m_outputTexture; }
UINT           IrradianceConvolution::GetSRVIndex() const { return m_outputTexture ? m_outputTexture->GetSRVIndex() : UINT_MAX; }
bool           IrradianceConvolution::IsGenerated() const { return m_isGenerated; }