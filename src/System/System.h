#pragma once
#define WIN32_LEAN_AND_MEAN
#include <memory>
#include <windows.h>

class Input;
class Window;
class Timer;
class CPU;
class FPS;
class Renderer;
class ImGuiManager;

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
    bool Frame();

private:
    std::unique_ptr<Window>       m_Window;
	std::unique_ptr<Input>        m_Input;
	std::unique_ptr<Timer>        m_Timer;
    std::unique_ptr<CPU>          m_CPU;
	std::unique_ptr<FPS>          m_FPS;
	std::unique_ptr<Renderer>     m_Renderer;
    std::shared_ptr<ImGuiManager> m_ImGuiManager;
}; // System

extern System* ApplicationHandle;