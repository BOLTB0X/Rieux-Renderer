#pragma once

class RendererState {
public: // Screen
    static bool  FullScrren;
    static bool  VsyncEnable;
    static int   ScreenWidth;
    static int   ScreenHeight;
    static float ScreenDepth;
    static float ScreenNear;
    static float aspectRatio;

public: // 프레임 및 디스크립터
    static UINT FrameCount;
    static UINT KFrameCBVIndex;
    static UINT KLightCBVIndex;
    static UINT KReservedDescriptorCount;
    static UINT KSharedHeapCapacity;
}; // RendererState