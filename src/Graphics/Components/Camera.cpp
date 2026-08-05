#include "Pch.h"
#include "Camera.h"
// Utils
#include "SharedCommons.h"
#include "MathHelper.h"

using namespace DirectX;
using namespace SharedCommons;

Camera::Camera()
    : m_position(SharedCommons::DEFAULT_POSITION),
    m_rotation(SharedCommons::DEFAULT_ROTATION),
    m_up(0.0f, 0.0f, 0.0f),
    m_fov(0.0f), m_near(0.0f), m_far(0.0f), m_aspect(0.0f) {
    m_maxPitch = SharedCommons::MAX_PITCH;
    m_minPitch = SharedCommons::MIN_PITCH;
    m_maxFov = SharedCommons::MAX_FOV;
    m_minFov = SharedCommons::MIN_FOV;
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();
	m_cullingProjectionMatrix = XMMatrixIdentity();
    m_forward = MathHelper::FRONT;
    m_right = DirectX::XMVector3Cross(m_forward, MathHelper::UP);
    m_upVector = DirectX::XMVector3Cross(m_right, m_forward);
    m_rotationSpeed = 0.5f;
    m_moveSpeed = 1.0;
    m_zoomSpeed = 1.0;
} //Camera

Camera::~Camera() {
} // ~Camera

void Camera::Init(float fov, float aspect, float screenNear, float screenFar) {
    m_fov = fov;
    m_aspect = aspect;
    m_near = screenNear;
    m_far = screenFar;

    UpdateProjection();
    Update();

    return;
} // Init

void Camera::UpdateProjection() {
    float fovRadian = XMConvertToRadians(m_fov);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(fovRadian, m_aspect, m_far, m_near);
    m_cullingProjectionMatrix = XMMatrixPerspectiveFovLH(fovRadian, m_aspect, m_near, m_far);
    //m_projectionMatrix = XMMatrixPerspectiveFovLH(fovRadian, m_aspect, m_near, m_far);
} // UpdateProjection

void Camera::Frame(float moveForward, float moveRight, float moveUp, float rotationDeltaX, float rotationDeltaY, float zoomDelta) {
    if (rotationDeltaX != 0.0f) {
        AddYaw(rotationDeltaX * m_rotationSpeed);
    }
    if (rotationDeltaY != 0.0f) {
        AddPitch(rotationDeltaY * m_rotationSpeed);
    }

    if (moveForward != 0.0f) {
        MoveForwardBack(moveForward * m_moveSpeed);
    }
    if (moveRight != 0.0f) {
        MoveLeftRight(moveRight * m_moveSpeed);
    }
    if (moveUp != 0.0f) {
        MoveUpDown(moveUp * m_moveSpeed);
    }

    if (zoomDelta != 0.0f) {
        float fovDelta = -zoomDelta * 0.05f;
        AddFOV(fovDelta * m_zoomSpeed);
    }

    Update();
} // Frame

void Camera::Update() {
    // 회전 행렬 계산 (Pitch, Yaw, Roll)
    XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m_rotation.x),
        XMConvertToRadians(m_rotation.y),
        XMConvertToRadians(m_rotation.z)
    );

    // 카메라가 바라보는 방향 계산
    m_forward = XMVector3TransformCoord(MathHelper::FRONT, rotationMatrix);
    m_upVector = XMVector3TransformCoord(MathHelper::UP, rotationMatrix);
    m_right = XMVector3Cross(m_upVector, m_forward);
    XMVECTOR pos = DirectX::XMLoadFloat3(&m_position);

    XMVECTOR lookAt = pos + m_forward;
    m_viewMatrix = XMMatrixLookAtLH(pos, lookAt, m_upVector);
    UpdateProjection();
    // 절두체 업데이트
} // Update

void Camera::OnGUI() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

    if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
        Reset();
    }
    ImGui::PopStyleColor(3);
    ImGui::Separator();

    DirectX::XMFLOAT3 pos = GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
        SetPosition(pos);
    }

    DirectX::XMFLOAT3 rot = GetRotation();
    if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f, -360.0f, 360.0f)) {
        SetRotation(rot);
    }

    ImGui::Separator();

    float fov = GetFov();
    if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f, "%.1f deg")) {
        SetFov(fov);
    }

    float nearP = GetNear();
    float farP = GetFar();
    ImGui::Text("Near: %.2f / Far: %.2f", nearP, farP);

    ImGui::Separator();
    ImGui::Text("Camera Speeds");

    ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.01f, 100.0f, "%.2f");
    ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.001f, 0.001f, 5.0f, "%.3f");
    ImGui::DragFloat("Zoom Speed", &m_zoomSpeed, 0.1f, 0.01f, 50.0f, "%.2f");

    Update();
} // onGui

void Camera::AddRotation(float pitch, float yaw) {
    AddPitch(pitch);
    AddYaw(yaw);
} // AddRotation

void Camera::AddPitch(float pitch) {
    m_rotation.x += pitch;
    m_rotation.x = MathHelper::clamp(m_rotation.x, m_minPitch, m_maxPitch);
} // AddPitch

void Camera::AddYaw(float yaw) {
    m_rotation.y += yaw;
    m_rotation.y = MathHelper::RotationWrap(m_rotation.y);
} // AddYaw

void Camera::AddFOV(float fovDelta) {
    m_fov += fovDelta;
    m_fov = MathHelper::clamp(m_fov, m_minFov, m_maxFov);
    UpdateProjection();
} // AddFOV

void Camera::Reset() {
    m_position = SharedCommons::DEFAULT_POSITION;
    m_rotation = SharedCommons::DEFAULT_ROTATION;
    m_fov = SharedCommons::DEFAULT_FOV;
    UpdateProjection();
} // Reset

void Camera::MoveForwardBack(float distance) {
    XMVECTOR forward = GetForwardVector();
    XMVECTOR pos = XMLoadFloat3(&m_position);
    pos = XMVectorMultiplyAdd(XMVectorReplicate(distance), forward, pos);
    XMStoreFloat3(&m_position, pos);
} // MoveForwardBack

void Camera::MoveLeftRight(float distance) {
    XMVECTOR right = GetRightVector();
    XMVECTOR pos = XMLoadFloat3(&m_position);
    pos = XMVectorMultiplyAdd(XMVectorReplicate(distance), right, pos);
    XMStoreFloat3(&m_position, pos);
} // MoveLeftRight

void Camera::MoveUpDown(float distance) {
    XMVECTOR up = GetUpVector();
    XMVECTOR pos = DirectX::XMLoadFloat3(&m_position);
    pos = XMVectorMultiplyAdd(XMVectorReplicate(distance), up, pos);
    XMStoreFloat3(&m_position, pos);
} // MoveUpDown

void Camera::SetPosition(const XMFLOAT3& pos) { m_position = pos; }
void Camera::SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
void Camera::SetRotation(const XMFLOAT3& rot) { m_rotation = rot; }
void Camera::SetRotation(float x, float y, float z) { m_rotation = { x, y, z }; }

// 투영 관련 Setters 
void Camera::SetFov(float fov) { m_fov = fov; UpdateProjection(); }
void Camera::SetAspect(float aspect) { m_aspect = aspect; UpdateProjection(); }
void Camera::SetNear(float screenNear) { m_near = screenNear; UpdateProjection(); }
void Camera::SetFar(float screenFar) { m_far = screenFar; UpdateProjection(); }

// Getters
XMFLOAT3 Camera::GetPosition() const { return m_position; }
XMFLOAT3 Camera::GetRotation() const { return m_rotation; }
XMMATRIX Camera::GetViewMatrix() const { return m_viewMatrix; }
XMMATRIX Camera::GetProjectionMatrix() const { return m_projectionMatrix; }
XMMATRIX Camera::GetCullingProjectionMatrix() const { return m_cullingProjectionMatrix; }

float Camera::GetFov() const { return m_fov; }
float Camera::GetNear() const { return m_near; }
float Camera::GetFar() const { return m_far; }
float Camera::GetAspect() const { return m_aspect; }

XMVECTOR Camera::GetForwardVector() const {
    XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
        MathHelper::ToRadians(m_rotation.x),
        MathHelper::ToRadians(m_rotation.y),
        MathHelper::ToRadians(m_rotation.z)
    );
    return DirectX::XMVector3TransformCoord(MathHelper::FRONT, rotationMatrix);
} // GetForwardVector

XMVECTOR Camera::GetRightVector() const {
    XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
        MathHelper::ToRadians(m_rotation.x),
        MathHelper::ToRadians(m_rotation.y),
        MathHelper::ToRadians(m_rotation.z)
    );
    return DirectX::XMVector3TransformCoord(MathHelper::RIGHT, rotationMatrix);
} // GetRightVector

XMVECTOR Camera::GetUpVector() const {
    XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
        MathHelper::ToRadians(m_rotation.x),
        MathHelper::ToRadians(m_rotation.y),
        MathHelper::ToRadians(m_rotation.z)
    );
    return DirectX::XMVector3TransformCoord(MathHelper::UP, rotationMatrix);
} // GetUpVector