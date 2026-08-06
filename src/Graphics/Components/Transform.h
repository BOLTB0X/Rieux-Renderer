#pragma once
#include <DirectXMath.h>
#include <d3d12.h>

class Transform {
public:
    Transform();
    Transform(const Transform&) = delete;
    Transform& operator=(const Transform&) = delete;
    ~Transform();

public:
    void              SetPosition(const DirectX::XMFLOAT3&);
    void              SetPosition(float, float, float);
    void              SetRotation(const DirectX::XMFLOAT3&);
    void              SetRotation(float, float, float);
    void              SetScale(const DirectX::XMFLOAT3&);
    void              SetScale(float, float, float);

    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetRotation() const;
    DirectX::XMFLOAT3 GetScale()    const;
    DirectX::XMMATRIX GetWorldMatrix();

    void              Translate(const DirectX::XMFLOAT3&);
    void              Translate(float, float, float);
    void              Rotate(const DirectX::XMFLOAT3&);
    void              Rotate(float, float, float);

private:
    void UpdateWorldMatrix();

private:
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_rotation;
    DirectX::XMFLOAT3 m_scale;
    DirectX::XMMATRIX m_worldMatrix;
    bool              m_isDirty;
}; // Transform