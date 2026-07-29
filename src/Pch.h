#pragma once

// ==========================================
// 매크로 충돌 방지 설정
// ==========================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX // Windows.h의 min, max 매크로를 비활성화하여 STL과 충돌 방지
#endif

// ==========================================
// C/C++ 표준 라이브러리
// ==========================================
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <functional>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <future>
#include <mutex>
#include <chrono>
#include <exception>
#include <cstdint>

// ==========================================
// Windows 및 DirectX 핵심
// ==========================================
#include <windows.h>
#include <objbase.h>
#include <crtdbg.h>
#include <wrl/client.h>

// DirectX 12
#include <d3d12.h>
#include "d3dx12.h"

// DXGI 및 기타 SDK 헤더
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

// ==========================================
// DirectXTK12
// ==========================================
#include <SimpleMath.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <DescriptorHeap.h>
#include <ResourceUploadBatch.h>
#include <CommonStates.h>

// ==========================================
// Assimp
// ==========================================
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ==========================================
// Third Party (spdlog, imgui)
// ==========================================
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>