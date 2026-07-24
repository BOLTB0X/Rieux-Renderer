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
UINT RendererState::FrameCBV = 0;     // 루트 파라미터 0 (b0)
UINT RendererState::LightCBV = 1;     // 루트 파라미터 1 (b1)
UINT RendererState::WorldIndex = 2;   // 루트 파라미터 2 (b2, 루트 상수)
UINT RendererState::Tex0Index = 3;    // 루트 파라미터 3 (t0, 알베도 테이블)
UINT RendererState::Tex1Index = 4;    // 루트 파라미터 4 (t1, 노멀 테이블)
UINT RendererState::Tex2Index = 5;    // 루트 파라미터 5 (t2, 알파 테이블)
UINT RendererState::ReservedDescriptorCount = 2;
UINT RendererState::SharedHeapCapacity = 1024;

UINT RendererState::RTVCapacity = 64;
UINT RendererState::DSVCapacity = 16;

DXGI_FORMAT RendererState::RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;