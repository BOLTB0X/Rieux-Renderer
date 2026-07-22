#include "Pch.h"
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"

using namespace DebugHelper;

DescriptorHeapAllocator::DescriptorHeapAllocator()
    : m_descriptorSize(0), m_capacity(0), m_nextFreeIndex(0), m_shaderVisible(false) {
} // DescriptorHeapAllocator

DescriptorHeapAllocator::~DescriptorHeapAllocator() {
} // ~DescriptorHeapAllocator

bool DescriptorHeapAllocator::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, bool shaderVisible) {
    m_capacity = capacity;
    m_shaderVisible = shaderVisible;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = capacity;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)))) {
        DebugPrint("DescriptorHeap 생성 실패");
        return false;
    }

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
    m_nextFreeIndex = 0;

    return true;
} // Init

UINT DescriptorHeapAllocator::Allocate() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_freeList.empty()) {
        UINT index = m_freeList.back();
        m_freeList.pop_back();
        return index;
    }

    if (m_nextFreeIndex >= m_capacity) {
        DebugPrint("DescriptorHeapAllocator 용량 초과 - capacity를 늘려야 함");
        return UINT_MAX;
    }

    return m_nextFreeIndex++;
} // Allocate

void DescriptorHeapAllocator::Free(UINT index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_freeList.push_back(index);
} // Free

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapAllocator::GetCPUHandle(UINT index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_descriptorSize;
    return handle;
} // GetCPUHandle

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapAllocator::GetGPUHandle(UINT index) const {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * m_descriptorSize;
    return handle;
} // GetGPUHandle

ID3D12DescriptorHeap* DescriptorHeapAllocator::GetHeap() const {
    return m_heap.Get();
} // GetHeap

UINT DescriptorHeapAllocator::GetCapacity() const {
    return m_capacity;
} // GetCapacity