// CascadedShadowMap.cpp
#include "Pch.h"
#include "CascadedShadowMap.h"
// Core
#include "RendererState.h"

using namespace DirectX;

CascadedShadowMap::CascadedShadowMap()
    : m_cascadeCount(4), m_shadowMapSize(2048), m_splitLambda(0.5f),
    m_splitDepths{}, m_cascadeViewProj{}, m_cascadeView{}, m_cascadeProj{}, m_splitDistancesOut(0, 0, 0, 0),
    m_isDirty(true), m_prevLightDir(0.0f, 0.0f, 0.0f), m_prevFov(0.0f), m_prevNearZ(0.0f), m_prevFarZ(0.0f) {
    for (auto& m : m_cascadeViewProj) m = XMMatrixIdentity();
    for (auto& m : m_cascadeView) m = XMMatrixIdentity();
    for (auto& m : m_cascadeProj) m = XMMatrixIdentity();
    for (auto& m : m_prevSnappedOrigin) m = {0.0f, 0.0f};
    for (auto& m : m_prevOrthoSize) m = -1.0f;
    for (auto& m : m_cascadeDirty) m = false;
    XMStoreFloat4x4(&m_prevCameraView, XMMatrixIdentity());
} // CascadedShadowMap

CascadedShadowMap::~CascadedShadowMap() {
} // ~CascadedShadowMap

bool CascadedShadowMap::Init(const InitParams& params) {
    m_cascadeCount = params.cascadeCount;
    m_shadowMapSize = params.shadowMapSize;
    m_splitLambda = params.splitLambda;
    return true;
} // Init

void CascadedShadowMap::Frame(const FrameParams& param) {
    XMFLOAT4X4 currentCameraView;
    XMStoreFloat4x4(&currentCameraView, param.camerViewMatrix);

    m_isDirty = false;

    if (memcmp(&currentCameraView, &m_prevCameraView, sizeof(XMFLOAT4X4)) != 0 ||
        abs(param.lightDir.x - m_prevLightDir.x) > 0.001f ||
        abs(param.lightDir.y - m_prevLightDir.y) > 0.001f ||
        abs(param.lightDir.z - m_prevLightDir.z) > 0.001f)
    {
        m_isDirty = true;
        m_prevCameraView = currentCameraView;
        m_prevLightDir = param.lightDir;
        m_prevFov = param.fov;
        m_prevNearZ = param.nearZ;
        m_prevFarZ = param.farZ;
    }

    if (!m_isDirty) {
        return;
    }

    ComputeSplits(param.nearZ, param.farZ);

    for (UINT i = 0; i < m_cascadeCount; ++i) {
        ComputeCascadeMatrix(i, param.fov, param.camerViewMatrix, param.lightDir, m_splitDepths[i], m_splitDepths[i + 1]);
    }
} // Frame

void CascadedShadowMap::ComputeSplits(const float& nearZ, const float& farZ) {
    // Practical Split Scheme (Zhang et al.) - log/uniform 블렌드
    m_splitDepths[0] = nearZ;
    for (UINT i = 1; i < m_cascadeCount; ++i) {
        float p = static_cast<float>(i) / static_cast<float>(m_cascadeCount);
        float logSplit = nearZ * powf(farZ / nearZ, p);
        float uniformSplit = nearZ + (farZ - nearZ) * p;
        m_splitDepths[i] = m_splitLambda * logSplit + (1.0f - m_splitLambda) * uniformSplit;
    }
    m_splitDepths[m_cascadeCount] = farZ;

    m_splitDistancesOut = XMFLOAT4(
        m_cascadeCount > 0 ? m_splitDepths[1] : farZ,
        m_cascadeCount > 1 ? m_splitDepths[2] : farZ,
        m_cascadeCount > 2 ? m_splitDepths[3] : farZ,
        farZ);
} // ComputeSplits

void CascadedShadowMap::ComputeCascadeMatrix(UINT index, const float& cameraFov, const DirectX::XMMATRIX& cameraView,
    const DirectX::XMFLOAT3& lightDir, float splitNear, float splitFar) {
    XMMATRIX sliceProj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(cameraFov), RendererState::aspectRatio, splitNear, splitFar);
    XMMATRIX sliceViewProj = cameraView * sliceProj;
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, sliceViewProj);

    XMVECTOR worldCorners[8];
    XMVECTOR center = XMVectorZero();
    for (int i = 0; i < 8; ++i) {
        XMVECTOR ndc = XMLoadFloat3(&ndcCorners[i]);
        worldCorners[i] = XMVector3TransformCoord(ndc, invViewProj);
        center += worldCorners[i];
    }
    center /= 8.0f;

    // 라이트 뷰행렬
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&lightDir));
    float sphereRadius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float dist = XMVectorGetX(XMVector3Length(worldCorners[i] - center));
        sphereRadius = std::max(sphereRadius, dist);
    }
    sphereRadius = ceilf(sphereRadius * 16.0f) / 16.0f;

    XMVECTOR upVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetY(dir)) > 0.999f) upVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    const float eyePullBack = sphereRadius * 4.0f;
    XMVECTOR eye = center - dir * eyePullBack;
    XMMATRIX candidateView = XMMatrixLookAtLH(eye, center, upVector);

    const float orthoSize = sphereRadius * 4.0f;
    float worldUnitsPerTexel = orthoSize / static_cast<float>(m_shadowMapSize);

    XMVECTOR originLS = XMVector3TransformCoord(XMVectorZero(), candidateView);
    float snappedX = floorf(XMVectorGetX(originLS) / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;
    float snappedY = floorf(XMVectorGetY(originLS) / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;
    XMVECTOR snapOffset = XMVectorSet(snappedX - XMVectorGetX(originLS), snappedY - XMVectorGetY(originLS), 0.0f, 0.0f);
    candidateView = candidateView * XMMatrixTranslationFromVector(snapOffset);

    // 실제로 바뀌었는지 체크
    const float orthoSizeEpsilon = 0.01f;
    bool originChanged = (fabsf(snappedX - m_prevSnappedOrigin[index].x) > 0.0001f) ||
        (fabsf(snappedY - m_prevSnappedOrigin[index].y) > 0.0001f);
    bool orthoSizeChanged = fabsf(orthoSize - m_prevOrthoSize[index]) > orthoSizeEpsilon;

    m_cascadeDirty[index] = originChanged || orthoSizeChanged;

    if (!m_cascadeDirty[index]) {
        return;
    }

    m_prevSnappedOrigin[index] = XMFLOAT2(snappedX, snappedY);
    m_prevOrthoSize[index] = orthoSize;

    float farPlane = eyePullBack + sphereRadius * 4.0f;
    XMMATRIX lightProj = XMMatrixOrthographicLH(orthoSize, orthoSize, 0.0f, farPlane);

    m_cascadeView[index] = candidateView;
    m_cascadeProj[index] = lightProj;
    m_cascadeViewProj[index] = candidateView * lightProj;
} // ComputeCascadeMatrix

UINT               CascadedShadowMap::GetCascadeCount() const { return m_cascadeCount; }
XMMATRIX           CascadedShadowMap::GetCascadeView(UINT index) const { return m_cascadeView[index]; }
XMMATRIX           CascadedShadowMap::GetCascadeProj(UINT index) const { return m_cascadeProj[index]; }
bool               CascadedShadowMap::GetCascadeDirty(UINT index) const { return m_cascadeDirty[index]; }
bool               CascadedShadowMap::IsDirty() const { return m_isDirty; }

XMMATRIX                                                       CascadedShadowMap::GetCascadeViewProj(UINT index) const { return m_cascadeViewProj[index]; }
std::array<DirectX::XMMATRIX, CascadedShadowMap::MAX_CASCADES> CascadedShadowMap::GetCascadeView() const { return m_cascadeView; }
std::array<DirectX::XMMATRIX, CascadedShadowMap::MAX_CASCADES> CascadedShadowMap::GetCascadeProj() const { return m_cascadeProj; }
const XMFLOAT4&                                                CascadedShadowMap::GetSplitDistances() const { return m_splitDistancesOut; }