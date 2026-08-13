#include "Pch.h"
#include "RendererState.h"
// Untils
#include "SharedCommons.h"

bool  RendererState::FullScrren = SharedCommons::FULL_SCREEN;
bool  RendererState::VsyncEnable = SharedCommons::VSYNC_ENABLED;
int   RendererState::ScreenWidth = SharedCommons::SCREEN_WIDTH;
int   RendererState::ScreenHeight = SharedCommons::SCREEN_HEIGHT;
float RendererState::ScreenDepth = SharedCommons::SCREEN_DEPTH;
float RendererState::ScreenNear = SharedCommons::SCREEN_NEAR;
float RendererState::aspectRatio = static_cast<float>(SharedCommons::SCREEN_WIDTH) / static_cast<float>(SharedCommons::SCREEN_HEIGHT);

UINT  RendererState::FrameCount = 2;
UINT  RendererState::FrameCBIndex = 0;
UINT  RendererState::LightCBIndex = 1;
UINT  RendererState::DebugLineFrameIndex = 0;
UINT  RendererState::WorldIndex = 2;
UINT  RendererState::Tex0Index = 3;
UINT  RendererState::Tex1Index = 4;
UINT  RendererState::Tex2Index = 5;
UINT  RendererState::MeshDataIndex = 3;
UINT  RendererState::MaterialIndicesIndex = 4;
UINT  RendererState::InstanceIndexParam = 3;
UINT  RendererState::InstanceDataIndex = 4;
UINT  RendererState::VertexBufferIndexParam = 5;
UINT  RendererState::BindlessTexIndex = 6;
UINT  RendererState::BindlessBufIndex = 7;
UINT  RendererState::ShadowMapIndex = 8;

UINT  RendererState::CullingFrustumPlanesIndex = 3;
UINT  RendererState::CullingMasterInstanceIndex = 4;
UINT  RendererState::CullingMasterCommandsIndex = 5;
UINT  RendererState::CullingVisibleMainCommandsIndex = 6;
UINT  RendererState::CullingVisibleVaseCommandsIndex = 7;
UINT  RendererState::CullingMainCountIndex = 8;
UINT  RendererState::CullingVaseCountIndex = 9;

UINT  RendererState::HZBConstantsIndex = 10;
UINT  RendererState::DepthTextureIndex = 11;
UINT  RendererState::HZBTextureIndex = 12;

UINT  RendererState::OcclusionConstantsIndex = 10;
UINT  RendererState::OcclusionHiZTextureIndex = 11;
UINT  RendererState::OcclusionFrustumMainCommandsIndex = 12;
UINT  RendererState::OcclusionFrustumVaseCommandsIndex = 13;
UINT  RendererState::OcclusionFrustumMainCountIndex = 14;
UINT  RendererState::OcclusionFrustumVaseCountIndex = 15;
UINT  RendererState::OcclusionMeshInstanceDataIndex = 16;
UINT  RendererState::OcclusionFinalMainCommandsIndex = 17;
UINT  RendererState::OcclusionFinalVaseCommandsIndex = 18;
UINT  RendererState::OcclusionFinalMainCountIndex = 19;
UINT  RendererState::OcclusionFinalVaseCountIndex = 20;

UINT RendererState::OcclusionCulledMainCommandsIndex = 21;
UINT RendererState::OcclusionCulledVaseCommandsIndex = 22;
UINT RendererState::OcclusionCulledMainCountIndex = 23;
UINT RendererState::OcclusionCulledVaseCountIndex = 24;
UINT RendererState::OcclusionPhaseIndex = 25;

UINT  RendererState::DebugCameraClipIndex = 11;
UINT  RendererState::DebugDepthTexIndex = 12;

UINT  RendererState::DebugFrameIndex = 11;
UINT  RendererState::DebugInstanceIndex = 12;
UINT  RendererState::DebugInstanceDataIndex = 13;

UINT  RendererState::SharedHeapCapacity = 1024;

UINT  RendererState::RTVCapacity = 64;
UINT  RendererState::DSVCapacity = 16;

UINT  RendererState::StaticSamplerIndex = 0;

DXGI_FORMAT RendererState::RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

RendererState::RendererState()
    : m_mappedFrameCB(nullptr), m_mappedSceneFrameCB(nullptr), m_mappedLightCB(nullptr) {
} // RendererState

RendererState::~RendererState() {
    Shutdown();
} // ~RendererState

RendererState::FrameParams::FrameParams() :
    view(DirectX::XMMatrixIdentity()),
    projection(DirectX::XMMatrixIdentity()),
    viewInv(DirectX::XMMatrixIdentity()),
    projInv(DirectX::XMMatrixIdentity()),
    cameraPosition(0.0f, 0.0f, 0.0f), cameraFov(0.0f),
    screenResolution((float)SharedCommons::SCREEN_WIDTH, (float)SharedCommons::SCREEN_HEIGHT),
    time(0.0f),
    direction(0.0f, -1.0f, 0.0f),
    ambient(0.2f, 0.2f, 0.2f, 1.0f),
    diffuse(1.0f, 1.0f, 1.0f, 1.0f),
    lookAt(0.0f, 0.0f, 0.0f),
    lightViewMatrix(DirectX::XMMatrixIdentity()),
    lightProjectionMatrix(DirectX::XMMatrixIdentity()),
    shadowMapWidth(0.0f),
    shadowMapHeight(0.0f), shadowBias(0.0f), shadowSpread(0.0f) {
} // FrameParams

bool RendererState::Init(ID3D12Device* device) {
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const UINT frameCBSize = (sizeof(GPUCommons::FrameCB) + 255) & ~255;
    const UINT lightCBSize = (sizeof(GPUCommons::DirectionalLightCB) + 255) & ~255;

    CD3DX12_RESOURCE_DESC frameCBDesc = CD3DX12_RESOURCE_DESC::Buffer(frameCBSize);
    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &frameCBDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_frameCB)))) {
        return false;
    }
    m_frameCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedFrameCB));

    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &frameCBDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_sceneFrameCB)))) {
        return false;
    }
    m_sceneFrameCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedSceneFrameCB));

    CD3DX12_RESOURCE_DESC lightCBDesc = CD3DX12_RESOURCE_DESC::Buffer(lightCBSize);
    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &lightCBDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_lightCB)))) {
        return false;
    }
    m_lightCB->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedLightCB));

    return true;
}

void RendererState::Frame(const FrameParams& frameParams) {
    GPUCommons::FrameCB frameData;
    frameData.view = XMMatrixTranspose(frameParams.view);
    frameData.projection = XMMatrixTranspose(frameParams.projection);
    frameData.cameraPosition = frameParams.cameraPosition;
    frameData.cameraFov = frameParams.cameraFov;

    GPUCommons::DirectionalLightCB lightData;
    lightData.direction = frameParams.direction;
    lightData.ambient = frameParams.ambient;
    lightData.diffuse = frameParams.diffuse;
    lightData.lookAt = frameParams.lookAt;
    lightData.lightViewMatrix = XMMatrixTranspose(frameParams.lightViewMatrix);
    lightData.lightProjectionMatrix = XMMatrixTranspose(frameParams.lightProjectionMatrix);
    lightData.shadowMapWidth = frameParams.shadowMapWidth;
    lightData.shadowMapHeight = frameParams.shadowMapHeight;
    lightData.shadowBias = frameParams.shadowBias;
    lightData.shadowSpread = frameParams.shadowSpread;
    
    // 매 프레임 갱신된 데이터를 Upload Heap에 복사
    if (m_mappedFrameCB) {
        memcpy(m_mappedFrameCB, &frameData, sizeof(GPUCommons::FrameCB));
    }
    if (m_mappedLightCB) {
        memcpy(m_mappedLightCB, &lightData, sizeof(GPUCommons::DirectionalLightCB));
    }
} // Frame

void RendererState::FrameScene(const FrameParams& frameParams) {
    GPUCommons::FrameCB frameData;
    frameData.view = XMMatrixTranspose(frameParams.view);
    frameData.projection = XMMatrixTranspose(frameParams.projection);
    frameData.cameraPosition = frameParams.cameraPosition;
    frameData.cameraFov = frameParams.cameraFov;

    if (m_mappedSceneFrameCB) {
        memcpy(m_mappedSceneFrameCB, &frameData, sizeof(GPUCommons::FrameCB));
    }
} // FrameScene

void RendererState::Shutdown() {
    if (m_frameCB) {
        m_frameCB->Unmap(0, nullptr);
        m_frameCB.Reset();
    }
    if (m_sceneFrameCB) {
        m_sceneFrameCB->Unmap(0, nullptr);
        m_sceneFrameCB.Reset();
    }
    if (m_lightCB) {
        m_lightCB->Unmap(0, nullptr);
        m_lightCB.Reset();
    }
    m_mappedFrameCB = nullptr;
    m_mappedSceneFrameCB = nullptr;
    m_mappedLightCB = nullptr;
} // Shutdown

D3D12_GPU_VIRTUAL_ADDRESS RendererState::GetFrameCBGPUVirtualAddress() const {
    return m_frameCB ? m_frameCB->GetGPUVirtualAddress() : 0;
} // GetFrameCBGPUVirtualAddress

D3D12_GPU_VIRTUAL_ADDRESS RendererState::GetSceneFrameCBGPUVirtualAddress() const {
    return m_sceneFrameCB ? m_sceneFrameCB->GetGPUVirtualAddress() : 0;
} // GetSceneFrameCBGPUVirtualAddress

D3D12_GPU_VIRTUAL_ADDRESS RendererState::GetLightCBGPUVirtualAddress() const {
    return m_lightCB ? m_lightCB->GetGPUVirtualAddress() : 0;
} // GetLightCBGPUVirtualAddress
