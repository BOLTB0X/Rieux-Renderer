#pragma once
#define WIN32_LEAN_AND_MEAN
#include <memory>
#include <windows.h>

class Window;
class Renderer;

class System {
public:
    System();
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    ~System();

    bool Init();
    void Shutdown();
    void Run();

    LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
    std::unique_ptr<Window>   m_Window;
	std::unique_ptr<Renderer> m_Renderer;
}; // System

extern System* ApplicationHandle;