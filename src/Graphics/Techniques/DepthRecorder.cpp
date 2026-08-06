#include "Pch.h"
#include "DepthRecorder.h"
// Core
#include "RendererState.h"
#include "D3D12PipelineState.h"
// D3D12
#include "RenderTexture.h"
#include "D3D12PipelineState.h"
#include "d3dx12.h"
// Utils
#include "DebugHelper.h"
#include "GPUCommons.h"

using namespace Microsoft::WRL;

DepthRecorder::DepthRecorder()
    : m_rootSignature(nullptr), m_solidDepthPSO(nullptr), m_alphaDepthPSO(nullptr) {
} // DepthRecorder

DepthRecorder::~DepthRecorder() {
    m_rootSignature = nullptr;
    m_solidDepthPSO = nullptr;
    m_alphaDepthPSO = nullptr;
} // ~DepthRecorder

bool DepthRecorder::Init(const InitParams& params) {
    if (!params.device || !params.rootSignature || !params.solidDepthPSO || !params.alphaDepthPSO) {
        DebugHelper::DebugPrint("DepthRecorder 초기화 실패: 유효하지 않은 파라미터");
        return false;
    }

    m_rootSignature = params.rootSignature;
    m_solidDepthPSO = params.solidDepthPSO;
    m_alphaDepthPSO = params.alphaDepthPSO;

    // 인다이렉트 커맨드 시그니처 생성 (Root Constant 1개 + Index Buffer View + Draw Indexed)
    D3D12_INDIRECT_ARGUMENT_DESC args[3] = {};

    // Arg 0: InstanceIndex (Root Constant)
    args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    args[0].Constant.RootParameterIndex = RendererState::InstanceIndexParam;
    args[0].Constant.DestOffsetIn32BitValues = 0;
    args[0].Constant.Num32BitValuesToSet = 1;

    // Arg 1: Index Buffer View
    args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

    // Arg 2: Draw Indexed Arguments
    args[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(GPUCommons::IndirectCommand);
    cmdSigDesc.NumArgumentDescs = 3;
    cmdSigDesc.pArgumentDescs = args;
    cmdSigDesc.NodeMask = 0;

    HRESULT hr = params.device->CreateCommandSignature(&cmdSigDesc, m_rootSignature, IID_PPV_ARGS(&m_commandSignature));
    if (FAILED(hr)) {
        DebugHelper::DebugPrint("DepthRecorder: CommandSignature 생성 실패");
        return false;
    }

    return true;
} // Init

void DepthRecorder::RecordDepthPre(const RecordParams& params) {
    if (!params.cmdList || !params.depthTexture || !params.mainIndirectBuffer || !params.vaseIndirectBuffer) {
        DebugHelper::DebugPrint("RecordDepthPre 실패: 유효하지 않은 파라미터");
        return;
    }

    ID3D12GraphicsCommandList* cmdList = params.cmdList;

    // 상태 전환: 뎁스 텍스처를 DEPTH_WRITE 상태로 변경
    params.depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    params.depthTexture->ClearDepth(cmdList, 0.0f, 0);

    //OMSetRenderTargets: Color RTV = 0개, DSV만 바인딩
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = params.depthTexture->GetDSVHandle();
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // Viewport & Scissor Rect 설정
    CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(params.depthTexture->GetWidth()), static_cast<float>(params.depthTexture->GetHeight()));
    CD3DX12_RECT scissorRect(0, 0, static_cast<LONG>(params.depthTexture->GetWidth()), static_cast<LONG>(params.depthTexture->GetHeight()));
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    // Root Signature 및 상수 버퍼 바인딩
    cmdList->SetGraphicsRootSignature(m_rootSignature);
    if (params.frameConstantsGPUAddress != 0) {
        cmdList->SetGraphicsRootConstantBufferView(RendererState::FrameCBIndex, params.frameConstantsGPUAddress);
    }

    // Pass 1: Solid Meshes
    if (params.mainCount > 0) {
        cmdList->SetPipelineState(m_solidDepthPSO->GetPSO());
        cmdList->ExecuteIndirect(m_commandSignature.Get(),
            params.mainCount, params.mainIndirectBuffer,
            0, nullptr, 0);
    }

    // Pass 2: Alpha-tested Meshes
    if (params.vaseCount > 0) {
        cmdList->SetPipelineState(m_alphaDepthPSO->GetPSO());
        cmdList->ExecuteIndirect(m_commandSignature.Get(),
            params.vaseCount, params.vaseIndirectBuffer,
            0, nullptr, 0);
    }

    params.depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
} // RecordDepthPre

void DepthRecorder::RecordShadowMap(const RecordParams& params) {
    if (!params.cmdList || !params.depthTexture || !params.mainIndirectBuffer || !params.vaseIndirectBuffer) {
        DebugHelper::DebugPrint("RecordShadowMap 실패: 유효하지 않은 파라미터");
        return;
    }

    ID3D12GraphicsCommandList* cmdList = params.cmdList;

    // 섀도우 맵 텍스처를 DEPTH_WRITE 상태로 전환
    params.depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear (Reverse-Z 기준 0.0f)
    params.depthTexture->ClearDepth(cmdList, 0.0f, 0);

    // DSV 바인딩
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = params.depthTexture->GetDSVHandle();
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // Viewport & Scissor Rect (섀도우 맵 해상도 기준)
    CD3DX12_VIEWPORT viewport(0.0f, 0.0f,
        static_cast<float>(params.depthTexture->GetWidth()), static_cast<float>(params.depthTexture->GetHeight()));
    CD3DX12_RECT scissorRect(0, 0,
        static_cast<LONG>(params.depthTexture->GetWidth()), static_cast<LONG>(params.depthTexture->GetHeight()));
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    cmdList->SetGraphicsRootSignature(m_rootSignature);
    if (params.lightConstantsGPUAddress != 0) {
        cmdList->SetGraphicsRootConstantBufferView(RendererState::LightCBIndex, params.lightConstantsGPUAddress);
    }

    if (params.mainCount > 0) {
        cmdList->SetPipelineState(m_solidDepthPSO->GetPSO());
        cmdList->ExecuteIndirect(m_commandSignature.Get(), params.mainCount, params.mainIndirectBuffer, 0, nullptr, 0);
    }

    if (params.vaseCount > 0) {
        cmdList->SetPipelineState(m_alphaDepthPSO->GetPSO());
        cmdList->ExecuteIndirect(m_commandSignature.Get(), params.vaseCount, params.vaseIndirectBuffer, 0, nullptr, 0);
    }

    // 픽셀 셰이더에서 그림자 샘플링이 가능하도록 PIXEL_SHADER_RESOURCE 상태로 변경
    params.depthTexture->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
} // RecordShadowMap