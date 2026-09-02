#include "Pch.h"
#include "EnvironmentProbe.h"
// Core
#include "RendererState.h"
#include "RenderTextureManager.h"
#include "RenderTexture.h"
#include "Transform.h"
// Utils
#include "GPUCommons.h"
#include "imgui.h"
#include "SharedCommons.h"

using namespace DirectX;
using namespace SharedCommons;

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
    : m_position(0, 0, 0), m_lastCapturedPos(0.0f, 0.0f, 0.0f), m_transform(std::make_unique<Transform>()),
    m_faceSize(256), m_isDirty(true),
    m_isInitialized(false), m_recaptureRadius(50.0f),
    m_projectionBoxMin(PROJ_BOX_MIN),
    m_projectionBoxMax(PROJ_BOX_MAX),
    m_influenceBoxMin(INFLUENCE_BOX_MIN),
	m_influenceBoxMax(INFLUENCE_BOX_MAX),
    m_blendDistance(30.0f),
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

    if (!m_isInitialized) {
        m_transform->SetPosition(param.cameraPos);
        m_position = m_transform->GetPosition();
        m_isDirty = true;
        m_isInitialized = true;
        UpdateFaceMatrices();
        return;
    }

    const DirectX::XMFLOAT3 transformPosition = m_transform->GetPosition();
    const float posThreshold = 0.001f;
    const bool posChanged =
        fabsf(transformPosition.x - m_position.x) > posThreshold ||
        fabsf(transformPosition.y - m_position.y) > posThreshold ||
        fabsf(transformPosition.z - m_position.z) > posThreshold;

    if (posChanged) {
        m_position = transformPosition;
        m_isDirty = true;
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

void EnvironmentProbe::OnGUI() {
    DirectX::XMFLOAT3 position = m_transform->GetPosition();
    if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
        m_transform->SetPosition(position);
    }

    ImGui::Separator();
    ImGui::Text("Projection Box");
    bool projectionChanged = false;
    projectionChanged |= ImGui::DragFloat3("Projection Min", &m_projectionBoxMin.x, 0.1f);
    projectionChanged |= ImGui::DragFloat3("Projection Max", &m_projectionBoxMax.x, 0.1f);

    ImGui::Text("Influence Box");
    bool influenceChanged = false;
    influenceChanged |= ImGui::DragFloat3("Influence Min", &m_influenceBoxMin.x, 0.1f);
    influenceChanged |= ImGui::DragFloat3("Influence Max", &m_influenceBoxMax.x, 0.1f);

    if (projectionChanged || influenceChanged) {
        m_projectionBoxMin.x = std::min(m_projectionBoxMin.x, m_projectionBoxMax.x);
        m_projectionBoxMin.y = std::min(m_projectionBoxMin.y, m_projectionBoxMax.y);
        m_projectionBoxMin.z = std::min(m_projectionBoxMin.z, m_projectionBoxMax.z);
        m_projectionBoxMax.x = std::max(m_projectionBoxMin.x, m_projectionBoxMax.x);
        m_projectionBoxMax.y = std::max(m_projectionBoxMin.y, m_projectionBoxMax.y);
        m_projectionBoxMax.z = std::max(m_projectionBoxMin.z, m_projectionBoxMax.z);

        m_influenceBoxMin.x = std::min(m_influenceBoxMin.x, m_influenceBoxMax.x);
        m_influenceBoxMin.y = std::min(m_influenceBoxMin.y, m_influenceBoxMax.y);
        m_influenceBoxMin.z = std::min(m_influenceBoxMin.z, m_influenceBoxMax.z);
        m_influenceBoxMax.x = std::max(m_influenceBoxMin.x, m_influenceBoxMax.x);
        m_influenceBoxMax.y = std::max(m_influenceBoxMin.y, m_influenceBoxMax.y);
        m_influenceBoxMax.z = std::max(m_influenceBoxMin.z, m_influenceBoxMax.z);
    }

    if (ImGui::Button("Generate Probe")) {
        m_isDirty = true;
    }
} // OnGUI

RenderTexture* EnvironmentProbe::GetCubemapTexture() const { return m_cubemapTexture; }
RenderTexture* EnvironmentProbe::GetDepthTexture() const { return m_depthTexture; }
UINT           EnvironmentProbe::GetFaceSize() const { return m_faceSize; }
XMFLOAT3       EnvironmentProbe::GetPosition() const { return m_position; }
XMFLOAT3       EnvironmentProbe::GetProjectionBoxMin() const { return m_projectionBoxMin; }
XMFLOAT3       EnvironmentProbe::GetProjectionBoxMax() const { return m_projectionBoxMax; }
XMFLOAT3       EnvironmentProbe::GetInfluenceBoxMin() const { return m_influenceBoxMin; }
XMFLOAT3       EnvironmentProbe::GetInfluenceBoxMax() const { return m_influenceBoxMax; }
float          EnvironmentProbe::GetBlendDistance() const { return m_blendDistance; }

D3D12_GPU_VIRTUAL_ADDRESS EnvironmentProbe::GetFaceFrameCBGPUAddress(UINT face) const {
    return m_faceFrameCB[face]->GetGPUVirtualAddress();
} // GetFaceFrameCBGPUAddress