#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace SharedConstants {
    /////////////////////////////////////////
    // 화면
    //////////////////////////////////////////
    static constexpr bool  FULL_SCREEN = false;
    static constexpr bool  VSYNC_ENABLED = true;
    static constexpr int   SCREEN_WIDTH = 800;
    static constexpr int   SCREEN_HEIGHT = 600;
    static constexpr float SCREEN_DEPTH = 8000.0f;
    static constexpr float SCREEN_NEAR = 0.1f;
    static constexpr int   CUBE_MAP_SIZE = 512;
    ////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////
	// 카메라
    static constexpr float MAX_PITCH = 89.0f;
    static constexpr float MIN_PITCH = -89.0f;

    static constexpr float MIN_FOV = 1.0f;
    static constexpr float MAX_FOV = 129.0f;

    static constexpr DirectX::XMFLOAT3 DEFAULT_POSITION = { 0.0f, 0.0f, -5.0f };
    static constexpr DirectX::XMFLOAT3 DEFAULT_ROTATION = { 0.0f, 0.0f, 0.0f };
    static constexpr float DEFAULT_FOV = 60.0f;
	////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////
	// 그림자
    static constexpr float SHADOW_VIEW_WIDTH = 2000.0f;
    static constexpr float SHADOW_VIEW_HEIGHT = 2000.0f;
    static constexpr float SHADOW_NEAR_Z = 0.1f;
    static constexpr float SHADOW_FAR_Z = 2000.0f;
    static constexpr float SHADOWMAP_WIDTH = 2048;
    static constexpr float SHADOWMAP_HEIGHT = 2048;
    ///////////////////////////////////////////////////////////////////////////////////////

} // SharedConstants

namespace SharedConstants {
    static constexpr DirectX::XMFLOAT3 LIGHT_DIR = { 0.0f, -1.0f, 0.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_DIFFUSE = { 255.0f * (3.0f / 255.0f), 250.0f * (3.0f / 255.0f), 245.0f * (3.0f / 255.0f), 1.0f };
    static constexpr DirectX::XMFLOAT4 LIGHT_AMBIENT = { 1.0f, 0.9f, 0.85f, 1.0f };
} // SharedConstants