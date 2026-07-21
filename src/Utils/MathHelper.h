#pragma once
#include <DirectXMath.h>

namespace MathHelper { // 수학 상수
    const float PI = 3.1415926535f;
    const float DEG_TO_RAD = PI / 180.0f;
    const float RAD_TO_DEG = 180.0f / PI;
} // 수학 상수

namespace MathHelper { // 기본 수학 함수

    inline float ToRadians(float degrees) { return degrees * DEG_TO_RAD; }
    inline float ToDegrees(float radians) { return radians * RAD_TO_DEG; }
    inline float Frac(float x) { return x - floorf(x); }
    inline float Lerp(float a, float b, float t) { return a + t * (b - a); }

    template<typename T>
    inline T Min(T a, T b) { return (a < b) ? a : b; }
    template<typename T>
    inline T Max(T a, T b) { return (a > b) ? a : b; }

    template <typename T>
    T clamp(T value, T min_val, T max_val) {
        if (value < min_val)
            return min_val;
        else if (value > max_val)
            return max_val;
        else
            return value;
    } // clamp

    template <typename T>
    inline T lerp(T a, T b, T t) {
        return (1 - t) * a + t * b;
    }

} // 기본 수학 함수

namespace MathHelper { // 행렬 상수
    inline const DirectX::XMMATRIX IDENTITY = DirectX::XMMatrixIdentity();
} // 행렬 상수

namespace MathHelper { // 벡터 상수
    inline const DirectX::XMVECTOR FRONT = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    inline const DirectX::XMVECTOR RIGHT = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    inline const DirectX::XMVECTOR UP = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
} // 벡터 상수

namespace MathHelper { // 벡터 함수
    inline DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    } // Add

    inline DirectX::XMFLOAT3 Multiply(const DirectX::XMFLOAT3& v, float s) {
        return { v.x * s, v.y * s, v.z * s };
    } // Multiply

    inline float Distance(float x1, float z1, float x2, float z2) {
        return sqrtf(powf(x2 - x1, 2) + powf(z2 - z1, 2));
    } // Distance

    inline float RotationWrap(float value) {
        value = fmod(value, 360.0);

        if (value < -180.0)
            value += 360.0;
        else if (value > 180.0)
            value -= 360.0;

        return value;
    } // RotationWrap

    inline DirectX::XMVECTOR RotationToVector(DirectX::XMFLOAT3 rot) {
        using namespace DirectX;

        // rotation값으로 회전 행렬을 만들고
        // 기본 forward 벡터와 곱해서 회전된 벡터를 계산
        float pitch = XMConvertToRadians(rot.x);
        float yaw = XMConvertToRadians(rot.y);
        float roll = XMConvertToRadians(rot.z);

        XMMATRIX rotationMat = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

        XMVECTOR direction = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // +Z forward
        direction = XMVector3Transform(direction, rotationMat);
        return direction;
    } // RotationToVector

    inline DirectX::XMFLOAT3 VectorToRotation(DirectX::XMFLOAT3 vec) {
        float pitch = -asinf(clamp<float>(vec.y, -1.0f, 1.0f));
        float yaw = atan2f(vec.x, vec.z);

        return DirectX::XMFLOAT3(DirectX::XMConvertToDegrees(pitch), DirectX::XMConvertToDegrees(yaw), 0.0f);
    } // VectorToRotation

    inline DirectX::XMMATRIX TransformUVRotationMatrix(float radians)
    {
        return DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * DirectX::XMMatrixRotationZ(radians) * DirectX::XMMatrixTranslation(0.5f, 0.5f, 0.0f);
    } // TransformUVRotationMatrix

    inline DirectX::XMMATRIX GetUVRotationMatrix(const DirectX::XMMATRIX& view)
    {
        using namespace DirectX;

        float m00 = XMVectorGetX(view.r[0]);
        float m01 = XMVectorGetY(view.r[0]);
        float camRot = atan2(m01, m00);

        return XMMatrixTranspose(TransformUVRotationMatrix(camRot));
    } // GetUVRotationMatrix

    inline DirectX::XMMATRIX GetBillboardMatrix(const DirectX::XMMATRIX& view)
    {
        using namespace DirectX;
        float m00 = XMVectorGetX(view.r[0]);
        float m01 = XMVectorGetY(view.r[0]);
        float camRot = atan2(m01, m00);
        return XMMatrixRotationY(-camRot);
    } // GetBillboardMatrix

} // 벡터 연산