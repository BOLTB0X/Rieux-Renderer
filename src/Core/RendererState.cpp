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

UINT RendererState::FrameCount = 2;
UINT RendererState::FrameCBIndex = 0;
UINT RendererState::LightCBIndex = 1;
UINT RendererState::WorldIndex = 2;
UINT RendererState::Tex0Index = 3;
UINT RendererState::Tex1Index = 4;
UINT RendererState::Tex2Index = 5;
UINT RendererState::MeshDataIndex = 3;
UINT RendererState::MaterialIndicesIndex = 4;
UINT RendererState::InstanceIndexParam = 3;
UINT RendererState::InstanceDataIndex = 4;
UINT RendererState::VertexBufferIndexParam = 5;
UINT RendererState::BindlessTexIndex = 6;
UINT RendererState::BindlessBufIndex = 7;

UINT RendererState::SharedHeapCapacity = 1024;

UINT RendererState::RTVCapacity = 64;
UINT RendererState::DSVCapacity = 16;

UINT RendererState::StaticSamplerIndex = 0;

DXGI_FORMAT RendererState::RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;