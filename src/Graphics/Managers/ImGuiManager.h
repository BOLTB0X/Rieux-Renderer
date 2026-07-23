#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <string>

class ImGuiWidget;

class ImGuiManager {
public:
    struct InitParams {
        HWND          hwnd;
        ID3D12Device* device;
        int           numFramesInFlight;
        DXGI_FORMAT   rtvFormat;

        InitParams() : hwnd(nullptr), device(nullptr), numFramesInFlight(0), rtvFormat{} {
        }
    }; // InitParams

public:
    ImGuiManager();
    ImGuiManager(const ImGuiManager&) = delete;
    ImGuiManager& operator=(const ImGuiManager&) = delete;
    ~ImGuiManager();

    bool Init(const InitParams&);
    void Shutdown();

    void Render();
    void RenderDrawData(ID3D12GraphicsCommandList*);

    bool CanControlWorld() const;
    void AddWidget(std::unique_ptr<ImGuiWidget>);
    void ToggleWidget();
    bool IsVisible() const;

    bool GetCameraLocked() const;
    void SetCameraLocked(bool);
    bool SetWorldClicked(bool) const;

private:
    bool                                         m_isInitialized;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    std::vector<std::unique_ptr<ImGuiWidget>>    m_widgets;
    bool                                         m_isCameraLocked;
    bool                                         m_showUI;
}; // ImGuiManager