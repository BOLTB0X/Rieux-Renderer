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
UINT RendererState::KFrameCBVIndex = 0;
UINT RendererState::KLightCBVIndex = 1;
UINT RendererState::KReservedDescriptorCount = 2;
UINT RendererState::KSharedHeapCapacity = 1024;