#include "Pch.h"
#include "System.h"
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "CPU.h"
#include "FPS.h"
// Core/
#include "Renderer.h"
#include "RendererState.h"
// Graphics
#include "Components/ImGuiManager.h"
// imgui
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

System* ApplicationHandle = nullptr;

// Imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Windows API에서 요구하는 전역 윈도우 프로시저 함수
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam)) {
        return true;
	}


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
	m_Input = std::make_unique<Input>();
	m_Timer = std::make_unique<Timer>();
	m_CPU = std::make_unique<CPU>();
	m_FPS = std::make_unique<FPS>();
	m_Renderer = std::make_unique<Renderer>();
    m_ImGuiManager = std::make_shared<ImGuiManager>();
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

    if (!m_Timer->Init()) {
        spdlog::error("Timer Init Failed!");
        return false;
    }

    if (!m_CPU->Init()) {
        spdlog::error("CPU Init Failed!");
        return false;
    }

    if (!m_FPS->Init()) {
        spdlog::error("FPS Init Failed!");
        return false;
    }

    if (!m_Input->Init(m_Window->GetHinstance(), m_Window->GetHwnd())) {
        spdlog::error("Input Init Failed!");
        return false;
	}

    Renderer::InitParams rendererParams;
    rendererParams.hwnd = m_Window->GetHwnd();
    rendererParams.width = RendererState::ScreenWidth;
    rendererParams.height = RendererState::ScreenHeight;
    rendererParams.imGuiManager = m_ImGuiManager;
    if (!m_Renderer->Init(rendererParams)) {
        spdlog::error("Renderer Init Failed!");
        return false;
	}

    return true;
} // Init

void System::Shutdown() {
    if (m_Renderer) {
        m_Renderer->Shutdown();
		m_Renderer.reset();
    }
    if (m_Input) {
		m_Input->Shutdown();
        m_Input.reset();
    }
    if (m_CPU) {
        m_CPU->Shutdown();
		m_CPU.reset();
    }
    if (m_FPS) {
        m_FPS.reset();
    }
    if (m_Timer) {
        m_Timer.reset();
    }
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
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (!Frame()) {
                break;
            }
        }
    }
} // Run

bool System::Frame() {
    if (!m_Input->Frame()) {
        return false;
    }

    if (m_Input->IsF1Toggled()) {
        m_ImGuiManager->ToggleWidget();

        bool uiVisible = m_ImGuiManager->IsVisible();
        m_Input->SetCursorHidden(!uiVisible);
    }

    m_FPS->Frame();
    m_CPU->Frame();
    m_Timer->Frame();

    Renderer::FrameParams frameParams = Renderer::FrameParams();
    frameParams.fps = m_FPS->GetFPS();
    frameParams.deltaTime = m_Timer->GetFrameTime();
    frameParams.playTime = m_Timer->GetTotalTime();
    frameParams.cpuPercentage = m_CPU->GetCPUPercentage();
    frameParams.cpuName = m_CPU->GetName();

    if (!m_Input->IsCursorHidden()) {
        if (!ImGui::GetIO().WantCaptureMouse) {

            if (m_Input->IsLeftMouseDown()) {
                auto rotDelta = m_Input->GetAdjustedMouseDelta();
                frameParams.rotationDeltaX = rotDelta.x;
                frameParams.rotationDeltaY = rotDelta.y;
            }

            int wheelDelta = m_Input->GetMouseWheelDelta();
            if (wheelDelta != 0) {
                frameParams.zoomDelta = static_cast<float>(wheelDelta);
            }
        }
    }

    float speed = 1.0f * frameParams.deltaTime;
    if (m_Input->IsWPressed()) {
        frameParams.moveForward += speed;
    }
    if (m_Input->IsSPressed()) {
        frameParams.moveForward -= speed;
    }
    if (m_Input->IsAPressed()) {
        frameParams.moveRight -= speed;
    }
    if (m_Input->IsDPressed()) {
        frameParams.moveRight += speed;
    }
    if (m_Input->IsZPressed()) {
        frameParams.moveUp += speed;
    }
    if (m_Input->IsXPressed()) {
        frameParams.moveUp -= speed;
    }

    return m_Renderer->Frame(frameParams);
} // Frame

// Windows 메시지 처리기
LRESULT CALLBACK System::MessageHandler(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam) {
    if (m_Input) {
        m_Input->UpdateMessage(umessage, wparam, lparam);
    }

    switch (umessage) {
    case WM_SETCURSOR:
        if (m_Input && m_Input->IsCursorHidden() && LOWORD(lparam) == HTCLIENT) {
            SetCursor(NULL);
            return TRUE;
        }
        break;
    }
    return DefWindowProc(hwnd, umessage, wparam, lparam);
} // MessageHandler