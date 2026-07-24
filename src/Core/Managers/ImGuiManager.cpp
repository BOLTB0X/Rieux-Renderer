#include "Pch.h"
#include "ImGuiManager.h"
// Utils
#include "ImGuiWidget.h"
#include "DebugHelper.h"
// imgui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

ImGuiManager::ImGuiManager()
    : m_isInitialized(false), m_isCameraLocked(false), m_showUI(true) {
} // ImGuiManager

ImGuiManager::~ImGuiManager() {
} // ~ImGuiManager

bool ImGuiManager::Init(const InitParams& initParams) {
    if (m_isInitialized) {
        return true;
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(initParams.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)))) {
        DebugHelper::DebugPrint("ImGui SRV Descriptor Heap 생성 실패");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(initParams.hwnd);
    ImGui_ImplDX12_Init(initParams.device, initParams.numFramesInFlight, initParams.rtvFormat,
        m_srvHeap.Get(),
        m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    m_isInitialized = true;
    return true;
} // Init

void ImGuiManager::Shutdown() {
    m_widgets.clear();

    if (m_isInitialized) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_isInitialized = false;
    }
} // Shutdown

void ImGuiManager::Render() {
    if (!m_isInitialized) {
        return;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 등록된 위젯들 실행
    if (m_showUI) {
        for (auto& widget : m_widgets) {
            if (widget->IsVisible()) {
                widget->Frame();
            }
        }
    }

    ImGui::Render();
} // Frame

void ImGuiManager::RenderDrawData(ID3D12GraphicsCommandList* commandList) {
    if (!m_isInitialized || !m_showUI) {
        return;
    }

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
    commandList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
} // Render

bool ImGuiManager::CanControlWorld() const {
    ImGuiIO& io = ImGui::GetIO();
    return !(io.WantCaptureMouse || io.WantCaptureKeyboard);
} // CanControlWorld

void ImGuiManager::AddWidget(std::unique_ptr<ImGuiWidget> widget) {
    m_widgets.push_back(std::move(widget));
    return;
} // AddWidget

void ImGuiManager::ToggleWidget() {
    m_showUI = !m_showUI;
    for (auto& widget : m_widgets) {
        widget->SetVisible(!widget->IsVisible());
    }
} // ToggleWidget

bool ImGuiManager::IsVisible() const {
    return m_showUI;
} // IsVisible

bool ImGuiManager::GetCameraLocked() const { 
    return m_isCameraLocked;
} // GetCameraLocked

void ImGuiManager::SetCameraLocked(bool lock) {
    m_isCameraLocked = lock;
} // SetCameraLocked

bool ImGuiManager::SetWorldClicked(bool mousePressed) const {
    return mousePressed && !ImGui::GetIO().WantCaptureMouse;
} // SetWorldClicked