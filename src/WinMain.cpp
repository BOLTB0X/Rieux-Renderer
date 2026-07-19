#include "Pch.h"
#include "System.h"
// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// STL
#include <vector>
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
    auto system = std::make_unique<System>();

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{ console_sink, msvc_sink };
    auto logger = std::make_shared<spdlog::logger>("Rieux", sinks.begin(), sinks.end());

    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%T] [%^%l%$] %v");
    spdlog::info("spdlog Initialized with MSVC Sink!");

    if (system->Init()) {
        spdlog::info("Rieux Renderer Started!");
        system->Run();
    }

    return 0;
} // WinMain