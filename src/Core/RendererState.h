#pragma once

class RendererState {
public:
    static bool  FullScrren;
    static bool  VsyncEnable;
    static int   ScreenWidth;
    static int   ScreenHeight;
    static float ScreenDepth;
    static float ScreenNear;
    static float aspectRatio;
}; // RendererState