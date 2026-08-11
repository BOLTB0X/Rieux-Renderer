#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <string>

class ImGuiWidget;
class DescriptorHeapAllocator;

class ImGuiManager {
public:
    struct InitParams {
        HWND          hwnd;
        ID3D12Device* device;
        int           numFramesInFlight;
        DXGI_FORMAT   rtvFormat;
		DescriptorHeapAllocator* heapAllocator;

        InitParams() : hwnd(nullptr), device(nullptr), numFramesInFlight(0), rtvFormat{}, heapAllocator(nullptr) {
        }
    }; // InitDefaultParams

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

    D3D12_GPU_DESCRIPTOR_HANDLE RegisterTexture(ID3D12Device*, ID3D12Resource*, DXGI_FORMAT);

private:
    bool                                         m_isInitialized;
    std::vector<std::unique_ptr<ImGuiWidget>>    m_widgets;
    bool                                         m_isCameraLocked;
    bool                                         m_showUI;
    DescriptorHeapAllocator*                     m_heapAllocator;
}; // ImGuiManager