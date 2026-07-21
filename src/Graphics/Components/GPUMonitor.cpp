#include "Pch.h"
#include "GPUMonitor.h"

GPUMonitor::GPUMonitor()
    : m_name("Unknown GPU"),
    m_vramUsageMB(0.0f),
    m_vramTotalMB(0.0f) {
} // GPU

GPUMonitor::~GPUMonitor() {
} // ~GPU

bool GPUMonitor::Init(IDXGIAdapter3* adapter) {
    if (!adapter) return false;

    m_adapter = adapter;

    DXGI_ADAPTER_DESC1 desc;
    if (SUCCEEDED(m_adapter->GetDesc1(&desc))) {
        std::wstring ws(desc.Description);
        m_name = std::string(ws.begin(), ws.end());
    }

    return true;
} // Init

void GPUMonitor::Shutdown() {
    m_adapter.Reset();
} // Shutdown

void GPUMonitor::Frame() {
    if (!m_adapter) {
        return;
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
    if (SUCCEEDED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo))) {
        m_vramUsageMB = static_cast<float>(videoMemoryInfo.CurrentUsage) / (1024.0f * 1024.0f);
        m_vramTotalMB = static_cast<float>(videoMemoryInfo.Budget) / (1024.0f * 1024.0f);
    }
} // Frame

const std::string& GPUMonitor::GetName() const {return m_name; }
float GPUMonitor::GetVRAMUsageMB() const { return m_vramUsageMB; }
float GPUMonitor::GetVRAMTotalMB() const { return m_vramTotalMB; }