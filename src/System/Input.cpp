#include "Pch.h"
#include "Input.h"
#include <windowsx.h> 
// Core
#include "RendererState.h"

Input::Input()
    : m_mouseX(0), m_mouseY(0), m_prevMouseX(0), m_prevMouseY(0),
    m_mouseDeltaX(0), m_mouseDeltaY(0), m_mouseWheelDelta(0),
    m_windowCenterX(0), m_windowCenterY(0),
    m_F1_released(true), m_F2_released(true), m_prevMouseL(false),
    m_sensitivity(0.05f), m_adjMouseX(0.0f), m_adjMouseY(0.0f),
    m_cursorHidden(false) {

    memset(m_keys, 0, sizeof(m_keys));
    memset(m_mouseButtons, 0, sizeof(m_mouseButtons));
} // Input

Input::~Input() {
} // ~Input

bool Input::Init(HINSTANCE hinstance, HWND hwnd) {
    m_hwnd = hwnd;

    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT pt = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    ClientToScreen(hwnd, &pt);

    m_windowCenterX = pt.x;
    m_windowCenterY = pt.y;

    m_cursorHidden = false;
    ShowCursor(TRUE);

    return true;
} // Init

void Input::Shutdown() {
    if (m_cursorHidden) {
        ShowCursor(TRUE);
    }
} // Shutdown

bool Input::Frame() {
    ProcessInput();

    m_prevMouseL = IsMouseLPressed();

    if (IsEscapePressed()) {
        return false;
    }

    m_mouseWheelDelta = 0;
    return true;
} // Frame

void Input::UpdateMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // 키보드 처리
    case WM_KEYDOWN:
        if (wParam < 256) m_keys[wParam] = true;
        break;
    case WM_KEYUP:
        if (wParam < 256) m_keys[wParam] = false;
        break;

        // 마우스 버튼 처리
    case WM_LBUTTONDOWN: m_mouseButtons[0] = true;  break;
    case WM_LBUTTONUP:   m_mouseButtons[0] = false; break;
    case WM_RBUTTONDOWN: m_mouseButtons[1] = true;  break;
    case WM_RBUTTONUP:   m_mouseButtons[1] = false; break;
    case WM_MBUTTONDOWN: m_mouseButtons[2] = true;  break;
    case WM_MBUTTONUP:   m_mouseButtons[2] = false; break;

        // 마우스 휠 처리
    case WM_MOUSEWHEEL:
        m_mouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
        break;

        // 마우스 이동 처리
    case WM_MOUSEMOVE:
        m_mouseX = GET_X_LPARAM(lParam);
        m_mouseY = GET_Y_LPARAM(lParam);
        break;
    }
} // UpdateMessage

void Input::ProcessInput() {
    m_mouseDeltaX = m_mouseX - m_prevMouseX;
    m_mouseDeltaY = m_mouseY - m_prevMouseY;

    m_prevMouseX = m_mouseX;
    m_prevMouseY = m_mouseY;

    m_adjMouseX = static_cast<float>(m_mouseDeltaX) * m_sensitivity;
    m_adjMouseY = static_cast<float>(m_mouseDeltaY) * m_sensitivity;

    // 화면 바깥으로 나가는 것 방지
    if (m_mouseX < 0) { m_mouseX = 0; }
    if (m_mouseY < 0) { m_mouseY = 0; }
    if (m_mouseX > RendererState::ScreenWidth) { m_mouseX = RendererState::ScreenWidth; }
    if (m_mouseY > RendererState::ScreenHeight) { m_mouseY = RendererState::ScreenHeight; }
} // ProcessInput

// 마우스 상태
void  Input::GetMouseLocation(int& mouseX, int& mouseY) { mouseX = m_mouseX; mouseY = m_mouseY; }
void  Input::GetMouseDelta(int& x, int& y) { x = m_mouseDeltaX; y = m_mouseDeltaY; }
int   Input::GetMouseWheelDelta() { return m_mouseWheelDelta; }
bool  Input::IsMouseLPressed() { return m_mouseButtons[0]; }
bool  Input::IsLeftMouseDown() { return m_mouseButtons[0]; }
bool  Input::IsRightMouseDown() { return m_mouseButtons[1]; }
float Input::GetSensitivity() const { return m_sensitivity; }
void  Input::SetSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
bool  Input::IsCursorHidden() const { return m_cursorHidden; }
Input::MouseDelta Input::GetAdjustedMouseDelta() const { return { m_adjMouseX, m_adjMouseY }; }

void Input::SetCursorHidden(bool hidden) {
    if (m_cursorHidden == hidden) return;

    m_cursorHidden = hidden;
    if (hidden) {
        while (ShowCursor(FALSE) >= 0);
    }
    else {
        while (ShowCursor(TRUE) < 0);
    }
} // SetCursorHidden

bool Input::IsEscapePressed() { return m_keys[VK_ESCAPE]; }
bool Input::IsWPressed() { return m_keys['W']; }
bool Input::IsAPressed() { return m_keys['A']; }
bool Input::IsSPressed() { return m_keys['S']; }
bool Input::IsDPressed() { return m_keys['D']; }
bool Input::IsZPressed() { return m_keys['Z']; }
bool Input::IsXPressed() { return m_keys['X']; }
bool Input::IsCPressed() { return m_keys['C']; }
bool Input::IsVPressed() { return m_keys['V']; }
bool Input::IsPgUpPressed() { return m_keys[VK_PRIOR]; } // PageUp
bool Input::IsPgDownPressed() { return m_keys[VK_NEXT]; }  // PageDown
bool Input::IsUpPressed() { return m_keys[VK_UP]; }
bool Input::IsDownPressed() { return m_keys[VK_DOWN]; }
bool Input::IsLeftPressed() { return m_keys[VK_LEFT]; }
bool Input::IsRightPressed() { return m_keys[VK_RIGHT]; }

bool Input::IsF1Toggled() {
    if (m_keys[VK_F1]) {
        if (m_F1_released) {
            m_F1_released = false;
            return true;
        }
    }
    else {
        m_F1_released = true;
    }
    return false;
} // IsF1Toggled

bool Input::IsF2Toggled() {
    if (m_keys[VK_F2]) {
        if (m_F2_released) {
            m_F2_released = false;
            return true;
        }
    }
    else {
        m_F2_released = true;
    }
    return false;
} // IsF2Toggled
