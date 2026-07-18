#include "Pch.h"
#include <windows.h>
// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// STL
#include <vector>
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{ console_sink, msvc_sink };
    auto logger = std::make_shared<spdlog::logger>("Rieux", sinks.begin(), sinks.end());

    // 로그 레벨 설정
    logger->set_level(spdlog::level::debug);
    // 글로벌 로거로 등록
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%T] [%^%l%$] %v");
    spdlog::info("spdlog Initialized with MSVC Sink!");

    MessageBoxW(nullptr, L"Rieux Renderer Started~!", L"Rieux Engine", MB_OK | MB_ICONINFORMATION);

    return 0;
} // WinMain