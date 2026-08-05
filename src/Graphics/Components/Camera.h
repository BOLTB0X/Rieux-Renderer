#pragma once
#include <memory>
#include <DirectXMath.h>

class Camera {
public:
    Camera();
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    ~Camera();

    void Init(float, float, float, float);
    void Frame(float, float, float, float, float, float);
    void Update();
    void OnGUI();

public:
    // 조작 함수
    void AddRotation(float, float);
    void AddPitch(float);
    void AddYaw(float);
    void AddFOV(float);
    void Reset();

    void MoveForwardBack(float);
    void MoveLeftRight(float);
    void MoveUpDown(float);

    // Setter & Getter
    void  SetPosition(const DirectX::XMFLOAT3&);
    void  SetPosition(float, float, float);
    void  SetRotation(const DirectX::XMFLOAT3&);
    void  SetRotation(float, float, float);
    void  SetFov(float);
    void  SetAspect(float);
    void  SetNear(float);
    void  SetFar(float);

    float             GetFov() const;
    float             GetNear() const;
    float             GetFar() const;
    float             GetAspect() const;
    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetRotation() const;
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;
    DirectX::XMMATRIX GetCullingProjectionMatrix() const;
    DirectX::XMVECTOR GetForwardVector() const;
    DirectX::XMVECTOR GetRightVector() const;
    DirectX::XMVECTOR GetUpVector() const;

private:
    void UpdateProjection();

private:
    DirectX::XMFLOAT3        m_position;
    DirectX::XMFLOAT3        m_rotation;
    DirectX::XMFLOAT3        m_up;
    DirectX::XMMATRIX        m_viewMatrix;
    DirectX::XMMATRIX        m_projectionMatrix;
    DirectX::XMMATRIX        m_cullingProjectionMatrix;
    DirectX::XMVECTOR        m_forward;
    DirectX::XMVECTOR        m_right;
    DirectX::XMVECTOR        m_upVector;

    float                    m_fov, m_near, m_far, m_aspect;
    float                    m_maxPitch, m_minPitch;
    float                    m_maxFov, m_minFov;
    float                    m_rotationSpeed;
    float                    m_moveSpeed;
    float                    m_zoomSpeed;
}; // Camera