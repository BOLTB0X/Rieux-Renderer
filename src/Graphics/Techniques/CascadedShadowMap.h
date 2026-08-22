#pragma once
#include <DirectXMath.h>
#include <wrl/client.h>
#include <array>

class CascadedShadowMap {
public:
    static const UINT MAX_CASCADES = 4;

    struct InitParams {
        UINT cascadeCount;   // 보통 4
        UINT shadowMapSize;  // 텍셀 스냅 계산용 (예: 2048)
        float splitLambda;   // log/uniform 블렌드 비율 (0.5 권장)

        InitParams() : cascadeCount(4), shadowMapSize(2048), splitLambda(0.5f) {
        }
    }; // InitParams

    struct FrameParams {
        float             nearZ;
        float             farZ;
        float             fov;
        DirectX::XMMATRIX camerViewMatrix;
        DirectX::XMFLOAT3 lightDir;

        FrameParams() : nearZ(0.0f), farZ(0.0f), fov(0.0f), 
            camerViewMatrix(DirectX::XMMatrixIdentity()),
            lightDir(0.0f, 0.0f, 0.0f) {
        }
    }; // FrameParams

public:

    CascadedShadowMap();
    CascadedShadowMap(const CascadedShadowMap&) = delete;
    CascadedShadowMap& operator=(const CascadedShadowMap&) = delete;
    ~CascadedShadowMap();

    bool Init(const InitParams&);
    void Frame(const FrameParams&);

public:
    UINT                      GetCascadeCount() const;
    DirectX::XMMATRIX         GetCascadeViewProj(UINT) const;
    const DirectX::XMFLOAT4&  GetSplitDistances() const;

private:
    void ComputeSplits(const float&, const float&);
    void ComputeCascadeMatrix(UINT, const float&, const DirectX::XMMATRIX&, const DirectX::XMFLOAT3&, float, float);

private:
    const DirectX::XMFLOAT3 ndcCorners[8] = {
        { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f },
        { 1.0f,  1.0f, 0.0f }, { -1.0f,  1.0f, 0.0f },
        { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f },
        { 1.0f,  1.0f, 1.0f }, { -1.0f,  1.0f, 1.0f }
    };

private:
    float                                       m_splitLambda;
    std::array<float, MAX_CASCADES + 1>         m_splitDepths;       // 뷰공간 거리 (N+1개 경계)
    std::array<DirectX::XMMATRIX, MAX_CASCADES> m_cascadeViewProj;
    DirectX::XMFLOAT4                           m_splitDistancesOut; // 셰이더로 넘길 4개 값
    UINT                                        m_cascadeCount;
    UINT                                        m_shadowMapSize;
}; // CascadedShadowMap