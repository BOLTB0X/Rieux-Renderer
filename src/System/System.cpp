#include "Pch.h"
#include "System.h"
#include "Window.h"
// Core/
#include "Renderer.h"
#include "RendererState.h"

System* ApplicationHandle = nullptr;

// Windows API에서 요구하는 전역 윈도우 프로시저 함수
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    switch (umessage) {
    case WM_DESTROY:
    case WM_CLOSE: {
        PostQuitMessage(0);
        return 0;
    }
    default: {
        return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
    }
    }
} // WndProc

System::System() {
    m_Window = std::make_unique<Window>();
	m_Renderer = std::make_unique<Renderer>();
    ApplicationHandle = this;
} // System

System::~System() {
    Shutdown();
} // System

bool System::Init() {
    if (!m_Window->Init(WndProc, L"Rieux Engine")) {
        spdlog::error("Window Init Failed!");
        return false;
    }

    if (!m_Renderer->Init(m_Window->GetHwnd(), RendererState::ScreenWidth, RendererState::ScreenHeight)) {
        spdlog::error("Renderer Init Failed!");
        return false;
	}

    return true;
} // Init

void System::Shutdown() {
    if (m_Window) {
        m_Window->Shutdown();
        m_Window.reset();
    }
    spdlog::info("System All Shutdown.");
} // Shutdown

void System::Run() {
    MSG msg = {};

    // 메인 메시지 루프
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            m_Renderer->Frame();
        }
    }
} // Run

// Windows 메시지 처리기
LRESULT CALLBACK System::MessageHandler(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    return DefWindowProc(hwnd, umessage, wparam, lparam);
} // MessageHandler