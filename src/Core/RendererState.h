#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

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
    static UINT CBV_Table;
    static UINT FrameCBIndex;
    static UINT LightCBIndex;
    static UINT WorldIndex;
    static UINT Tex0Index;
    static UINT Tex1Index;
    static UINT Tex2Index;
    static UINT StaticSamplerIndex;

    static UINT MeshDataIndex;
    static UINT MaterialIndicesIndex;
    static UINT VertexBufferIndexParam;
    static UINT BindlessTexIndex;
    static UINT BindlessBufIndex;

    static UINT SharedHeapCapacity;
    static UINT RTVCapacity;
    static UINT DSVCapacity;

public:
    static DXGI_FORMAT RTVFormat;
}; // RendererState