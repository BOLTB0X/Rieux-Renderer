// DebugHelper.h
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <wrl/client.h>
#include <windows.h>
#include <string>
#include <DirectXMath.h>
#include <spdlog/spdlog.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);

class VolumeTexture;

namespace DebugHelper { // HWND

    inline void ErrorBox(HWND hwnd, const std::wstring& message) {
        spdlog::error("Engine Error Box:");
        MessageBoxW(hwnd, message.c_str(), L"Engine Error", MB_OK | MB_ICONERROR);
    } // ErrorBox
} // DebugHelper

namespace DebugHelper { // 출력

    inline void DebugPrint(const std::string& message) {
        spdlog::debug(message);
    } // DebugPrint


    inline bool SuccessCheck(bool bSuccess, const std::string& msg) {
        if (bSuccess == true) return true;
        spdlog::error("{} (Status: Failed)", msg);
        return false;
    } // SuccessCheck
} // DebugHelper