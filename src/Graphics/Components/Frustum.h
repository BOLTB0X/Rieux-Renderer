#pragma once
#include <DirectXMath.h>

class Frustum {
public:
    Frustum();
    Frustum(const Frustum&) = delete;
    Frustum& operator=(const Frustum&) = delete;
    ~Frustum();

    void Init(float);
    void BuildFrustum(DirectX::XMMATRIX, DirectX::XMMATRIX);

public:
    bool CheckPoint(float, float, float);
    bool CheckCube(float, float, float, float);
    bool CheckSphere(float, float, float, float);
    bool CheckBoundingBox(float, float, float, float, float, float);
    bool CheckBoundingBoxMinMax(float, float, float, float, float, float);

private:
    float m_screenDepth;
    float m_planes[6][4];
}; // Frustum