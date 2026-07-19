#include "Pch.h"
#include "RendererState.h"
// Untils
#include "SharedConstants.h"


bool  RendererState::FullScrren = SharedConstants::FULL_SCREEN;
bool  RendererState::VsyncEnable = SharedConstants::VSYNC_ENABLED;
int   RendererState::ScreenWidth = SharedConstants::SCREEN_WIDTH;
int   RendererState::ScreenHeight = SharedConstants::SCREEN_HEIGHT;
float RendererState::ScreenDepth = SharedConstants::SCREEN_DEPTH;
float RendererState::ScreenNear = SharedConstants::SCREEN_NEAR;
float RendererState::aspectRatio = static_cast<float>(SharedConstants::SCREEN_WIDTH) / static_cast<float>(SharedConstants::SCREEN_HEIGHT);