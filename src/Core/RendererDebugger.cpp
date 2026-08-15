#include "Pch.h"
#include "RendererDebugger.h"
#include "RendererState.h"
// D3D12
#include "D3D12SwapChain.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
// Managers
#include "RenderTextureManager.h"
#include "PSOManager.h"
// Graphics/Tools
#include "DescriptorHeapAllocator.h"
// Components
#include "Camera.h"
// Techniques
#include "HierarchicalZBuffer.h"
#include "OcclusionCuller.h"
// World
#include "Sponza.h"
// Utils
#include "SharedCommons.h"
// PIX
#include <pix3.h>

using namespace DirectX;

RendererDebugger::RendererDebugger()
    : m_device(nullptr), m_rendererState(nullptr), m_renderTextureManager(nullptr),
    m_swapChain(nullptr), m_psoManager(nullptr), m_sharedDescriptorAllocator(nullptr),
    m_sceneCamera(nullptr), m_masterCamera(nullptr), m_occlusionCuller(nullptr),
    m_sponza(nullptr) {
} // RendererDebugger

RendererDebugger::~RendererDebugger() {
    Shutdown();
} // ~RendererDebugger

bool RendererDebugger::Init(const InitParams& params) {
    if (!params.device || !params.rendererState || !params.renderTextureManager ||
        !params.swapChain || !params.psoManager || !params.sharedDescriptorAllocator ||
        !params.sceneCamera || !params.masterCamera || !params.occlusionCuller || !params.sponza) {
        return false;
    }

    m_device = params.device;
    m_rendererState = params.rendererState;
    m_renderTextureManager = params.renderTextureManager;
    m_swapChain = params.swapChain;
    m_psoManager = params.psoManager;
    m_sharedDescriptorAllocator = params.sharedDescriptorAllocator;
    m_sceneCamera = params.sceneCamera;
    m_masterCamera = params.masterCamera;
    m_occlusionCuller = params.occlusionCuller;
    m_sponza = params.sponza;

    if (!BuildSceneCameraFrustumBuffer()) {
        return false;
    }

    return true;
} // Init

void RendererDebugger::Frame() {
    if (!m_rendererState->m_sceneCameraFrustumMappedData || !m_sceneCamera) {
        return;
    }

    const XMMATRIX viewProjection = m_sceneCamera->GetViewMatrix()
        * m_sceneCamera->GetStandardZProjectionMatrix();
    const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, viewProjection);

    const XMFLOAT3 ndcCorners[8] = {
        { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }
    };

    XMFLOAT3 worldCorners[8] = {};
    for (int i = 0; i < 8; ++i) {
        const XMVECTOR ndc = XMLoadFloat3(&ndcCorners[i]);
        XMStoreFloat3(&worldCorners[i], XMVector3TransformCoord(ndc, inverseViewProjection));
    }

    const XMFLOAT3 nearColor = { 1.0f, 0.2f, 0.1f };
    const XMFLOAT3 farColor = { 1.0f, 0.8f, 0.1f };
    auto writeLine = [this](UINT index, const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& color) {
        m_rendererState->m_sceneCameraFrustumMappedData[index * 2] = { a, color };
        m_rendererState->m_sceneCameraFrustumMappedData[index * 2 + 1] = { b, color };
        };

    writeLine(0, worldCorners[0], worldCorners[1], nearColor);
    writeLine(1, worldCorners[1], worldCorners[2], nearColor);
    writeLine(2, worldCorners[2], worldCorners[3], nearColor);
    writeLine(3, worldCorners[3], worldCorners[0], nearColor);
    writeLine(4, worldCorners[4], worldCorners[5], farColor);
    writeLine(5, worldCorners[5], worldCorners[6], farColor);
    writeLine(6, worldCorners[6], worldCorners[7], farColor);
    writeLine(7, worldCorners[7], worldCorners[4], farColor);
    writeLine(8, worldCorners[0], worldCorners[4], nearColor);
    writeLine(9, worldCorners[1], worldCorners[5], nearColor);
    writeLine(10, worldCorners[2], worldCorners[6], nearColor);
    writeLine(11, worldCorners[3], worldCorners[7], nearColor);
} // Frame

void RendererDebugger::Shutdown() {
    m_device = nullptr;
    m_rendererState = nullptr;
    m_renderTextureManager = nullptr;
    m_swapChain = nullptr;
    m_psoManager = nullptr;
    m_sharedDescriptorAllocator = nullptr;
    m_sceneCamera = nullptr;
    m_masterCamera = nullptr;
    m_occlusionCuller = nullptr;
    m_sponza = nullptr;
} // Shutdown

void RendererDebugger::DebugDepthRenderTextures(ID3D12GraphicsCommandList* cmdList, RenderTexture* depthTexture) {
    PIXBeginEvent(cmdList, PIX_COLOR(128, 128, 128), L"Depth Debug Pass");
    {
        auto debugDepthTex = m_renderTextureManager->CreateRenderTexture(
            "Depth_Debug",
            depthTexture->GetWidth(),
            depthTexture->GetHeight(),
            RenderTexture::RenderTextureType::Normal,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1).get();

        debugDepthTex->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE debugRtvHandle = debugDepthTex->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &debugRtvHandle, FALSE, nullptr);

        cmdList->RSSetViewports(1, &m_swapChain->GetViewport());
        cmdList->RSSetScissorRects(1, &m_swapChain->GetScissorRect());
        cmdList->SetGraphicsRootSignature(m_psoManager->GetID3D12RootSignature(SharedCommons::KEY_TRANS_REVERSE_Z_SIG));
        cmdList->SetPipelineState(m_psoManager->GetPSO(SharedCommons::KEY_TRANS_REVERSE_Z_PSO)->GetPSO());

        const Camera* activeCamera = GetActiveCamera();
        struct { float nearPlane; float farPlane; } cameraClip = { activeCamera->GetNear(), activeCamera->GetFar() };
        cmdList->SetGraphicsRoot32BitConstants(RendererState::DebugCameraClipIndex, 2, &cameraClip, 0);
        cmdList->SetGraphicsRootDescriptorTable(
            RendererState::DebugDepthTexIndex,
            m_sharedDescriptorAllocator->GetGPUHandle(depthTexture->GetSRVIndex()));
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        debugDepthTex->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    PIXEndEvent(cmdList);
} // DebugDepthRenderTextures

void RendererDebugger::DebugHiZRenderTextures(ID3D12GraphicsCommandList* cmdList) {
    HierarchicalZBuffer::BuildParams hizParams;
    hizParams.cmdList = cmdList;
    hizParams.depthTexture = m_renderTextureManager->GetRenderTexture(SharedCommons::KEY_DEPTH_RENDER_TEXTURE).get();
    hizParams.hizTexture = m_renderTextureManager->GetRenderTexture(SharedCommons::KEY_HIZ_DEPTH_RENDER_TEXTURE).get();

    PIXBeginEvent(cmdList, PIX_COLOR(128, 255, 128), L"Hi-Z Debug Pass");
    {
        auto hizTexture = hizParams.hizTexture;
        auto hizDebugTex = m_renderTextureManager->CreateRenderTexture(
            "HiZ_Depth_Debug",
            hizTexture->GetWidth(),
            hizTexture->GetHeight(),
            RenderTexture::RenderTextureType::Normal,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            1).get();

        hizTexture->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        hizDebugTex->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE hizDebugRtv = hizDebugTex->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &hizDebugRtv, FALSE, nullptr);

        D3D12_VIEWPORT hizViewport = { 0.0f, 0.0f, static_cast<float>(hizTexture->GetWidth()), static_cast<float>(hizTexture->GetHeight()), 0.0f, 1.0f };
        D3D12_RECT hizScissor = { 0, 0, static_cast<LONG>(hizTexture->GetWidth()), static_cast<LONG>(hizTexture->GetHeight()) };
        cmdList->RSSetViewports(1, &hizViewport);
        cmdList->RSSetScissorRects(1, &hizScissor);

        cmdList->SetGraphicsRootSignature(m_psoManager->GetID3D12RootSignature(SharedCommons::KEY_TRANS_REVERSE_Z_SIG));
        cmdList->SetPipelineState(m_psoManager->GetPSO(SharedCommons::KEY_TRANS_REVERSE_Z_PSO)->GetPSO());

        const Camera* activeCamera = GetActiveCamera();
        struct { float nearPlane; float farPlane; } cameraClip = { activeCamera->GetNear(), activeCamera->GetFar() };
        cmdList->SetGraphicsRoot32BitConstants(RendererState::DebugCameraClipIndex, 2, &cameraClip, 0);
        cmdList->SetGraphicsRootDescriptorTable(
            RendererState::DebugDepthTexIndex,
            m_sharedDescriptorAllocator->GetGPUHandle(hizTexture->GetSRVIndex()));
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        hizDebugTex->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    PIXEndEvent(cmdList);
} // DebugHiZRenderTextures

void RendererDebugger::DebugBoundingBox(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(0, 255, 0), L"Debug AABB Pass");

    Sponza::RenderDebugParams debugParams;
    debugParams.cmdList = cmdList;
    debugParams.frameConstantsGPUAddress = m_rendererState->GetFrameCBGPUVirtualAddress();
    debugParams.mainCmdBuffer = m_occlusionCuller->GetFinalMainCommandsBuffer();
    debugParams.mainCounter = m_occlusionCuller->GetFinalMainCounterBuffer();
    debugParams.vaseCmdBuffer = m_occlusionCuller->GetFinalVaseCommandsBuffer();
    debugParams.vaseCounter = m_occlusionCuller->GetFinalVaseCounterBuffer();
    m_sponza->RenderDebugAABB(debugParams);

    PIXEndEvent(cmdList);
} // DebugBoundingBox

Camera* RendererDebugger::GetActiveCamera() {
    return m_rendererState->IsUsingMasterCamera() ? m_masterCamera : m_sceneCamera;
} // GetActiveCamera

const Camera* RendererDebugger::GetActiveCamera() const {
    return m_rendererState->IsUsingMasterCamera() ? m_masterCamera : m_sceneCamera;
} // GetActiveCamera

bool RendererDebugger::BuildSceneCameraFrustumBuffer() {
    const UINT vertexCount = 24;
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(RendererState::DebugLineVertex) * vertexCount);

    if (FAILED(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_rendererState->m_sceneCameraFrustumBuffer)))) {
        return false;
    }

    if (FAILED(m_rendererState->m_sceneCameraFrustumBuffer->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&m_rendererState->m_sceneCameraFrustumMappedData)))) {
        m_rendererState->m_sceneCameraFrustumBuffer.Reset();
        return false;
    }

    m_rendererState->m_sceneCameraFrustumVBV.BufferLocation = m_rendererState->m_sceneCameraFrustumBuffer->GetGPUVirtualAddress();
    m_rendererState->m_sceneCameraFrustumVBV.SizeInBytes = sizeof(RendererState::DebugLineVertex) * vertexCount;
    m_rendererState->m_sceneCameraFrustumVBV.StrideInBytes = sizeof(RendererState::DebugLineVertex);
    Frame();
    return true;
} // BuildSceneCameraFrustumBuffer

void RendererDebugger::DebugSceneCameraFrustum(ID3D12GraphicsCommandList* cmdList) {
    if (!m_rendererState->IsUsingMasterCamera() || !m_rendererState->m_sceneCameraFrustumBuffer || !m_psoManager) {
        return;
    }

    ID3D12RootSignature* rootSignature = m_psoManager->GetID3D12RootSignature(
        SharedCommons::KEY_DEBUG_LINE_SIG);
    D3D12PipelineState* pipelineState = m_psoManager->GetPSO(
        SharedCommons::KEY_DEBUG_LINE_PSO);
    if (!rootSignature || !pipelineState) {
        return;
    }

    PIXBeginEvent(cmdList, PIX_COLOR(255, 80, 40), L"Scene Camera Frustum Debug");
    cmdList->SetGraphicsRootSignature(rootSignature);
    cmdList->SetPipelineState(pipelineState->GetPSO());
    cmdList->SetGraphicsRootConstantBufferView(RendererState::DebugLineFrameIndex, m_rendererState->GetFrameCBGPUVirtualAddress());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_rendererState->m_sceneCameraFrustumVBV);
    cmdList->DrawInstanced(24, 1, 0, 0);
    PIXEndEvent(cmdList);
} // DebugSceneCameraFrustum
