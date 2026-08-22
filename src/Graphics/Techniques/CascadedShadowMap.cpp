// CascadedShadowMap.cpp
#include "Pch.h"
#include "CascadedShadowMap.h"
// Core
#include "RendererState.h"

using namespace DirectX;

CascadedShadowMap::CascadedShadowMap()
    : m_cascadeCount(4), m_shadowMapSize(2048), m_splitLambda(0.5f),
    m_splitDepths{}, m_cascadeViewProj{}, m_splitDistancesOut(0, 0, 0, 0) {
    for (auto& m : m_cascadeViewProj) m = XMMatrixIdentity();
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


    ComputeSplits(param.nearZ, param.farZ);

    for (UINT i = 0; i < m_cascadeCount; ++i) {
        ComputeCascadeMatrix(i, param.fov, param.camerViewMatrix, param.lightDir, m_splitDepths[i], m_splitDepths[i + 1]);
    }
} // Update


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
    // 이 슬라이스만의 임시 투영행렬로 카메라 view*proj 역행렬 구성
    XMMATRIX sliceProj = XMMatrixPerspectiveFovLH(
        cameraFov, RendererState::aspectRatio, splitNear, splitFar);
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

    // 라이트 뷰행렬 (center를 바라보는 방향으로)
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&lightDir));
    float sphereRadius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float dist = XMVectorGetX(XMVector3Length(worldCorners[i] - center));
        sphereRadius = std::max(sphereRadius, dist);
    }
    sphereRadius = ceilf(sphereRadius * 16.0f) / 16.0f; // 안정화용 라운딩

    XMVECTOR upVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetY(dir)) > 0.999f) upVector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    XMVECTOR eye = center - dir * sphereRadius * 2.0f;
    XMMATRIX lightView = XMMatrixLookAtLH(eye, center, upVector);

    // 4) 텍셀 스냅 (shimmering 방지) - 라이트 공간에서 center를 텍셀 단위로 반올림
    float worldUnitsPerTexel = (sphereRadius * 2.0f) / static_cast<float>(m_shadowMapSize);
    XMVECTOR centerLS = XMVector3TransformCoord(center, lightView);
    float snappedX = floorf(XMVectorGetX(centerLS) / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;
    float snappedY = floorf(XMVectorGetY(centerLS) / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;
    XMVECTOR snapOffset = XMVectorSet(
        snappedX - XMVectorGetX(centerLS),
        snappedY - XMVectorGetY(centerLS),
        0.0f, 0.0f);
    // 라이트뷰 자체를 스냅된 만큼 다시 오프셋 (뷰 행렬을 라이트 로컬축 기준으로 이동)
    lightView = lightView * XMMatrixTranslationFromVector(snapOffset);

    // 정사영 - sphereRadius 기반이라 회전에도 안정적, near/far는 pull-back 포함해서 넉넉히
    float nearPlanePullBack = sphereRadius * 4.0f; // 프러스텀 밖 caster 포함용 여유
    XMMATRIX lightProj = XMMatrixOrthographicLH(
        sphereRadius * 2.0f, sphereRadius * 2.0f,
        0.0f, sphereRadius * 2.0f + nearPlanePullBack);

    m_cascadeViewProj[index] = lightView * lightProj;
} // ComputeCascadeMatrix

UINT               CascadedShadowMap::GetCascadeCount() const { return m_cascadeCount; }
XMMATRIX           CascadedShadowMap::GetCascadeViewProj(UINT index) const { return m_cascadeViewProj[index]; }
const XMFLOAT4&    CascadedShadowMap::GetSplitDistances() const { return m_splitDistancesOut; }