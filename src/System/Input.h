#pragma once
#include <windows.h>

class Input {
public:
    Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    ~Input();

    bool Init(HINSTANCE hinstance, HWND hwnd);
    void Shutdown();
    bool Frame();

public:
    struct MouseDelta {
        float x;
        float y;
    };

    bool IsMouseLPressed();
    bool IsLeftMouseDown();
    bool IsRightMouseDown();
    bool IsCursorHidden() const;

    bool IsEscapePressed();
    bool IsWPressed();
    bool IsAPressed();
    bool IsSPressed();
    bool IsDPressed();
    bool IsZPressed();
    bool IsXPressed();
    bool IsF1Toggled();

    bool IsPgUpPressed();
    bool IsPgDownPressed();

    void UpdateMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void       GetMouseLocation(int&, int&);
    void       GetMouseDelta(int&, int&);
    int        GetMouseWheelDelta();
    MouseDelta GetAdjustedMouseDelta() const;
    float      GetSensitivity() const;

    void       SetSensitivity(float);
    void       SetCursorHidden(bool);

private:
    void ProcessInput();

private:
    bool  m_keys[256];
    bool  m_mouseButtons[3]; // 0: Left, 1: Right, 2: Middle

    int   m_mouseX, m_mouseY;
    int   m_prevMouseX, m_prevMouseY;
    int   m_mouseDeltaX, m_mouseDeltaY;
    int   m_mouseWheelDelta;

    int   m_windowCenterX, m_windowCenterY;
    bool  m_F1_released;
    bool  m_prevMouseL;
    float m_sensitivity;
    float m_adjMouseX, m_adjMouseY;
    bool  m_cursorHidden;
}; // Input