#include "Pch.h"
#include "GPUMonitor.h"

GPUMonitor::GPUMonitor()
    : m_name("Unknown GPU"),
    m_vramUsageMB(0.0f), m_vramTotalMB(0.0f),
    m_gpuTickFrequency(0), m_maxQueries(0) {
} // GPU

GPUMonitor::~GPUMonitor() {
} // ~GPU

bool GPUMonitor::Init(const InitParams& parmas) {
    if (!parmas.adapter || !parmas.device || !parmas.commandQueue) {
        return false;
    }

    m_adapter = parmas.adapter;
    m_maxQueries = parmas.maxQueries;
    m_queryData.resize(m_maxQueries, 0);

    DXGI_ADAPTER_DESC1 desc;
    if (SUCCEEDED(m_adapter->GetDesc1(&desc))) {
        std::wstring ws(desc.Description);
        m_name = std::string(ws.begin(), ws.end());
    }

    // GPU 타임스탬프 주파수 얻기
    if (FAILED(parmas.commandQueue->GetTimestampFrequency(&m_gpuTickFrequency))) {
        return false;
    }

    // 쿼리 힙 생성
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Count = m_maxQueries;
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

    if (FAILED(parmas.device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_queryHeap)))) {
        return false;
    }

    // 결과를 CPU로 복사할 리드백 버퍼 생성
    CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_maxQueries * sizeof(uint64_t));

    if (FAILED(parmas.device->CreateCommittedResource(
        &readbackHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_queryResultBuffer)))) {
        return false;
    }
    return true;
} // Init

void GPUMonitor::Shutdown() {
    m_adapter.Reset();
    m_queryHeap.Reset();
    m_queryResultBuffer.Reset();
} // Shutdown

void GPUMonitor::Frame() {
    if (!m_adapter || !m_queryResultBuffer) {
        return;
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
    if (SUCCEEDED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo))) {
        m_vramUsageMB = static_cast<float>(videoMemoryInfo.CurrentUsage) / (1024.0f * 1024.0f);
        m_vramTotalMB = static_cast<float>(videoMemoryInfo.Budget) / (1024.0f * 1024.0f);
    }

    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, m_maxQueries * sizeof(uint64_t) };
    if (SUCCEEDED(m_queryResultBuffer->Map(0, &readRange, &mappedData))) {
        memcpy(m_queryData.data(), mappedData, m_maxQueries * sizeof(uint64_t));

        D3D12_RANGE writeRange = { 0, 0 };
        m_queryResultBuffer->Unmap(0, &writeRange);
    }
} // Frame

void GPUMonitor::RecordTimestamp(ID3D12GraphicsCommandList* commandList, uint32_t queryIndex) {
    if (queryIndex < m_maxQueries && m_queryHeap) {
        commandList->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
    }
} // RecordTimestamp

void GPUMonitor::ResolveQueryData(ID3D12GraphicsCommandList* commandList, uint32_t queryCount) {
    if (m_queryHeap && m_queryResultBuffer) {
        if (queryCount == 0 || queryCount > m_maxQueries) {
            queryCount = m_maxQueries;
        }
        commandList->ResolveQueryData(
            m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, queryCount,
            m_queryResultBuffer.Get(), 0);
    }
} // ResolveQueryData

const std::string& GPUMonitor::GetName() const {return m_name; }
float              GPUMonitor::GetVRAMUsageMB() const { return m_vramUsageMB; }
float              GPUMonitor::GetVRAMTotalMB() const { return m_vramTotalMB; }

double GPUMonitor::GetTimeMs(uint32_t startIndex, uint32_t endIndex) const {
    if (startIndex >= m_maxQueries || endIndex >= m_maxQueries || m_gpuTickFrequency == 0) {
        return 0.0;
    }

    uint64_t startTick = m_queryData[startIndex];
    uint64_t endTick = m_queryData[endIndex];

    if (endTick < startTick) return 0.0;

    return ((endTick - startTick) * 1000.0) / static_cast<double>(m_gpuTickFrequency);
} // GPUMonitor