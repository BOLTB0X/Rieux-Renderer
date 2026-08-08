#include "Pch.h"
#include "ImGuiManager.h"
// Components
#include "DescriptorHeapAllocator.h"
// Utils
#include "ImGuiWidget.h"
#include "DebugHelper.h"
// imgui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

ImGuiManager::ImGuiManager()
    : m_isInitialized(false), m_isCameraLocked(false), m_showUI(true), m_heapAllocator(nullptr) {
} // ImGuiManager

ImGuiManager::~ImGuiManager() {
    Shutdown();
	m_heapAllocator = nullptr;
} // ~ImGuiManager

bool ImGuiManager::Init(const InitParams& initParams) {
    if (!initParams.device || !initParams.hwnd || !initParams.heapAllocator) {
        // OutputDebugStringA("ImGuiManager::Init - 잘못된 파라미터 (Null 포인터)!\n");
        return false;
    }

    if (m_isInitialized) {
        return true;
    }

	m_heapAllocator = initParams.heapAllocator;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    UINT fontSrvIndex = m_heapAllocator->Allocate();
    D3D12_CPU_DESCRIPTOR_HANDLE fontCpuHandle = m_heapAllocator->GetCPUHandle(fontSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE fontGpuHandle = m_heapAllocator->GetGPUHandle(fontSrvIndex);

    ImGui_ImplWin32_Init(initParams.hwnd);
    ImGui_ImplDX12_Init(initParams.device, initParams.numFramesInFlight, initParams.rtvFormat,
        m_heapAllocator->GetHeap(),
        fontCpuHandle,
        fontGpuHandle);

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
    m_isInitialized = false;
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

    ID3D12DescriptorHeap* heaps[] = { m_heapAllocator->GetHeap() };
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

D3D12_GPU_DESCRIPTOR_HANDLE ImGuiManager::RegisterTexture(ID3D12Device* device, ID3D12Resource* resource, DXGI_FORMAT srvFormat) {
    UINT slot = m_heapAllocator->Allocate(); // ImGui 전용 힙에서 슬롯 할당

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srvFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(resource, &srvDesc, m_heapAllocator->GetCPUHandle(slot));
    return m_heapAllocator->GetGPUHandle(slot);
} // RegisterTexture