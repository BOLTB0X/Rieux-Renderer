#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <mutex>

class DescriptorHeapAllocator {
public:
    DescriptorHeapAllocator();
    DescriptorHeapAllocator(const DescriptorHeapAllocator&) = delete;
    DescriptorHeapAllocator& operator=(const DescriptorHeapAllocator&) = delete;
    ~DescriptorHeapAllocator();

    bool Init(ID3D12Device*, D3D12_DESCRIPTOR_HEAP_TYPE, UINT, bool);
    UINT Allocate();
    void Free(UINT index);

public:
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const;
    ID3D12DescriptorHeap*       GetHeap() const;
    UINT                        GetCapacity() const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    UINT                                         m_descriptorSize;
    UINT                                         m_capacity;
    UINT                                         m_nextFreeIndex;
    std::vector<UINT>                            m_freeList;
    bool                                         m_shaderVisible;
    std::mutex                                   m_mutex;
}; // DescriptorHeapAllocator