#pragma once
#include <windows.h>
#include <string>

class Window {
public:
    Window();
    Window(const Window& other) = delete;
    Window& operator=(const Window& other) = delete;
    ~Window();

    bool      Init(WNDPROC, LPCWSTR);
    void      Shutdown();

    HWND      GetHwnd()      const;
    HINSTANCE GetHinstance() const;

private:
    HWND      m_hwnd;
    HINSTANCE m_hinstance;
    LPCWSTR   m_engineName;
}; // Window