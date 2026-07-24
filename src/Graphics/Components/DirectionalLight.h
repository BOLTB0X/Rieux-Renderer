#pragma once
#include <wrl/client.h>
#include <DirectXMath.h>

class DirectionalLight {
public:
    DirectionalLight();
    DirectionalLight(const DirectionalLight&) = delete;
    DirectionalLight& operator=(const DirectionalLight&) = delete;
    ~DirectionalLight();

    void Init();
    void Update();
    void OnGUI();

    // Setter
    void              SetLookAt(DirectX::XMFLOAT3);
    void              SetLookAt(float, float, float);
    // Getter
    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetDirection() const;
    DirectX::XMFLOAT4 GetDiffuse() const;
    DirectX::XMFLOAT4 GetAmbient() const;
    DirectX::XMFLOAT3 GetLookAt() const;
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjection() const;
    DirectX::XMFLOAT2 GetUV(const DirectX::XMMATRIX&, const DirectX::XMMATRIX&) const;

private:
    DirectX::XMFLOAT3 m_direction;
    DirectX::XMFLOAT4 m_ambient;
    DirectX::XMFLOAT4 m_diffuse;
    DirectX::XMFLOAT3 m_lookAt;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
}; // DirectionalLight