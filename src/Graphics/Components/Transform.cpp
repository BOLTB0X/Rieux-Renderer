#include "Pch.h"
#include "Transform.h"

using namespace DirectX;

Transform::Transform()
    : m_position(0, 0, 0), m_rotation(0, 0, 0), m_scale(1, 1, 1),
    m_isDirty(true) {
    m_worldMatrix = XMMatrixIdentity();
} // Transform

Transform::~Transform() {
} // ~Transform

void Transform::SetPosition(const XMFLOAT3& pos) {
    m_position = pos;
    m_isDirty = true;
} // SetPosition

void Transform::SetPosition(float x, float y, float z) {
    m_position = { x, y, z };
    m_isDirty = true;
} // SetPosition

void Transform::SetRotation(const XMFLOAT3& rot) {
    m_rotation = rot;
    m_isDirty = true;
} // SetRotation

void Transform::SetRotation(float x, float y, float z) {
    m_rotation = { x, y, z };
    m_isDirty = true;
} // SetRotation

void Transform::SetScale(const XMFLOAT3& scale) {
    m_scale = scale;
    m_isDirty = true;
} // SetScale

void Transform::SetScale(float x, float y, float z) {
    m_scale = { x, y, z };
    m_isDirty = true;
} // SetScale

DirectX::XMFLOAT3 Transform::GetPosition() const {
    return m_position;
} // GetPosition

DirectX::XMFLOAT3 Transform::GetRotation() const {
    return m_rotation;
} // GetRotation

DirectX::XMFLOAT3 Transform::GetScale()    const {
    return m_scale;
} // GetScale

XMMATRIX Transform::GetWorldMatrix()  {
    if (m_isDirty) {
        UpdateWorldMatrix();
    }
    return m_worldMatrix;
} // GetWorldMatrix

void Transform::Translate(const XMFLOAT3& delta) {
    m_position.x += delta.x;
    m_position.y += delta.y;
    m_position.z += delta.z;
    m_isDirty = true;
} // Translate

void Transform::Translate(float x, float y, float z) {
    m_position.x += x;
    m_position.y += y;
    m_position.z += z;
    m_isDirty = true;
} // Translate

void Transform::Rotate(const XMFLOAT3& delta) {
    m_rotation.x += delta.x;
    m_rotation.y += delta.y;
    m_rotation.z += delta.z;
    m_isDirty = true;
} // Rotate

void Transform::Rotate(float pitch, float yaw, float roll) {
    m_rotation.x += pitch;
    m_rotation.y += yaw;
    m_rotation.z += roll;
    m_isDirty = true;
} // Translate

void Transform::UpdateWorldMatrix() {
    XMMATRIX s = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

    XMMATRIX r = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m_rotation.x),
        XMConvertToRadians(m_rotation.y),
        XMConvertToRadians(m_rotation.z)
    );

    XMMATRIX t = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    m_worldMatrix = s * r * t;
    m_isDirty = false;
} // UpdateWorldMatrix