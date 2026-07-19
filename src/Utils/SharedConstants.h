#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace SharedConstants {
    static constexpr bool  FULL_SCREEN = false;
    static constexpr bool  VSYNC_ENABLED = true;
    static constexpr int   SCREEN_WIDTH = 800;
    static constexpr int   SCREEN_HEIGHT = 600;
    static constexpr float SCREEN_DEPTH = 8000.0f;
    static constexpr float SCREEN_NEAR = 0.1f;
    static constexpr int   CUBE_MAP_SIZE = 512;
} // SharedConstants