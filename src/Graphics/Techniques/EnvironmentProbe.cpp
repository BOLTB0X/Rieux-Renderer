#include "Pch.h"
#include "EnvironmentProbe.h"
// Core
#include "RendererState.h"
#include "RenderTextureManager.h"
#include "RenderTexture.h"
// Utils
#include "GPUCommons.h"

using namespace DirectX;

static const XMVECTOR kFaceForward[6] = {
    XMVectorSet(1, 0, 0, 0), XMVectorSet(-1, 0, 0, 0),
    XMVectorSet(0, 1, 0, 0), XMVectorSet(0,-1, 0, 0),
    XMVectorSet(0, 0, 1, 0), XMVectorSet(0, 0,-1, 0),
};

static const XMVECTOR kFaceUp[6] = {
    XMVectorSet(0, 1, 0, 0), XMVectorSet(0, 1, 0, 0),
    XMVectorSet(0, 0,-1, 0), XMVectorSet(0, 0, 1, 0),
    XMVectorSet(0, 1, 0, 0), XMVectorSet(0, 1, 0, 0),
};

EnvironmentProbe::EnvironmentProbe()
    : m_position(0, 0, 0), m_prevCameraPos(0, 0, 0), m_prevCameraDir(0, 0, 0),
    m_faceSize(256), m_isDirty(true),
    m_cubemapTexture(nullptr), m_depthTexture(nullptr),
    m_faceViewMatrices{}, m_faceProjMatrix(XMMatrixIdentity()),
    m_mappedFaceFrameCB{} {
} // EnvironmentProbe

EnvironmentProbe::~EnvironmentProbe() {
    for (UINT i = 0; i < FACE_COUNT; ++i) {
        if (m_faceFrameCB[i]) m_faceFrameCB[i]->Unmap(0, nullptr);
    }
} // ~EnvironmentProbe

bool EnvironmentProbe::Init(const InitParams& params) {
    m_faceSize = params.faceSize;

    auto tex = params.renderTextureManager->CreateRenderTexture(
        "EnvironmentProbe_Cubemap", m_faceSize, m_faceSize,
        RenderTexture::RenderTextureType::Normal, params.format, 1, FACE_COUNT, true);
    if (!tex) return false;
    m_cubemapTexture = tex.get();

    auto depth = params.renderTextureManager->CreateRenderTexture(
        "EnvironmentProbe_Depth", m_faceSize, m_faceSize,
        RenderTexture::RenderTextureType::Depth, DXGI_FORMAT_UNKNOWN, 1);
    if (!depth) return false;
    m_depthTexture = depth.get();

    m_faceProjMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 4000.0f, 0.1f);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    const UINT cbSize = (sizeof(GPUCommons::FrameCB) + 255) & ~255;
    CD3DX12_RESOURCE_DESC cbDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

    for (UINT i = 0; i < FACE_COUNT; ++i) {
        if (FAILED(params.device->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_faceFrameCB[i])))) {
            return false;
        }
        m_faceFrameCB[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedFaceFrameCB[i]));
    }

    return true;
} // Init

void EnvironmentProbe::Frame(const FrameParams& param) {
    m_isDirty = false;

    const float posThreshold = 0.001f;
    const float dirThreshold = 0.001f;

    bool posChanged = (fabsf(param.cameraPos.x - m_prevCameraPos.x) > posThreshold) ||
        (fabsf(param.cameraPos.y - m_prevCameraPos.y) > posThreshold) ||
        (fabsf(param.cameraPos.z - m_prevCameraPos.z) > posThreshold);

    if (posChanged) {
        m_isDirty = true;
        SetPosition(param.cameraPos);

        m_prevCameraPos = param.cameraPos;
        m_prevCameraDir = param.cameraDir;

        UpdateFaceMatrices();
    }
} // Frame

void EnvironmentProbe::SetPosition(const XMFLOAT3& pos) {
    m_position = pos;
} // SetPosition

void EnvironmentProbe::UpdateFaceMatrices() {
    XMVECTOR eye = XMLoadFloat3(&m_position);

    for (UINT face = 0; face < FACE_COUNT; ++face) {
        XMVECTOR target = eye + kFaceForward[face];
        m_faceViewMatrices[face] = XMMatrixLookAtLH(eye, target, kFaceUp[face]);

        GPUCommons::FrameCB frameData;
        frameData.view = XMMatrixTranspose(m_faceViewMatrices[face]);
        frameData.projection = XMMatrixTranspose(m_faceProjMatrix);
        frameData.cameraPosition = m_position;
        frameData.cameraFov = XM_PIDIV2;

        memcpy(m_mappedFaceFrameCB[face], &frameData, sizeof(GPUCommons::FrameCB));
    }
} // UpdateFaceMatrices

void           EnvironmentProbe::MarkDirty() { m_isDirty = true; }
bool           EnvironmentProbe::IsDirty() const { return m_isDirty; }
void           EnvironmentProbe::ClearDirty() { m_isDirty = false; }
RenderTexture* EnvironmentProbe::GetCubemapTexture() const { return m_cubemapTexture; }
RenderTexture* EnvironmentProbe::GetDepthTexture() const { return m_depthTexture; }
UINT           EnvironmentProbe::GetFaceSize() const { return m_faceSize; }

D3D12_GPU_VIRTUAL_ADDRESS EnvironmentProbe::GetFaceFrameCBGPUAddress(UINT face) const {
    return m_faceFrameCB[face]->GetGPUVirtualAddress();
} // GetFaceFrameCBGPUAddress