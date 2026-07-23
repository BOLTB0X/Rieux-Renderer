#include "Pch.h"
#include "Window.h"
// Core
#include "RendererState.h"
// Utils
#include "SharedCommons.h"

using namespace SharedCommons;

Window::Window()
    : m_hwnd(nullptr),
    m_hinstance(nullptr),
    m_engineName(nullptr) {
} // Window

Window::~Window() {
    Shutdown();
} // ~Window

bool Window::Init(WNDPROC wndProc, LPCWSTR appName) {
    m_engineName = appName;
    m_hinstance = GetModuleHandle(NULL);

    WNDCLASSEXW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hinstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = m_engineName;
    wc.cbSize = sizeof(WNDCLASSEX);

    if (!RegisterClassExW(&wc)) return false;

    int posX, posY;
    int windowWidth = SharedCommons::SCREEN_WIDTH;
    int windowHeight = SharedCommons::SCREEN_HEIGHT;

    if (SharedCommons::FULL_SCREEN) {
        posX = posY = 0;
    }
    else {
        RECT windowRect = { 0, 0, (LONG)SharedCommons::SCREEN_WIDTH, (LONG)SharedCommons::SCREEN_HEIGHT };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        windowWidth = windowRect.right - windowRect.left;
        windowHeight = windowRect.bottom - windowRect.top;

        posX = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
        posY = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;
    }

    m_hwnd = CreateWindowExW(WS_EX_APPWINDOW, m_engineName, m_engineName,
        WS_OVERLAPPEDWINDOW, posX, posY, windowWidth, windowHeight,
        NULL, NULL, m_hinstance, NULL);

    if (!m_hwnd)
        return false;

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    RendererState::ScreenWidth = clientRect.right - clientRect.left;
    RendererState::ScreenHeight = clientRect.bottom - clientRect.top;

    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
    ShowCursor(true);

    spdlog::info("Window Init: {}x{}", RendererState::ScreenWidth, RendererState::ScreenHeight);
    return true;
} // Init

void Window::Shutdown() {
    ShowCursor(true);

    if (SharedCommons::FULL_SCREEN) {
        ChangeDisplaySettings(NULL, 0);
    }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    if (m_engineName) {
        UnregisterClassW(m_engineName, m_hinstance);
        m_engineName = nullptr;
    }

    m_hinstance = nullptr;
} // Shutdown

HWND Window::GetHwnd() const { 
    return m_hwnd;
} // GetHwnd

HINSTANCE Window::GetHinstance() const {
    return m_hinstance;
} // GetHinstance