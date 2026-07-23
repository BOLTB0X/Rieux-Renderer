#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

class CommandQueue {
public:
    CommandQueue();
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;
    ~CommandQueue();

    bool Init(ID3D12Device*);
    void Shutdown();
    void Reset();

    void Execute();
    void WaitForPreviousFrame();

public:
    ID3D12CommandQueue*        GetQueue() const;
    ID3D12GraphicsCommandList* GetList() const;
    ID3D12CommandAllocator*    GetAllocator() const;

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>         m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>  m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                m_fence;
    UINT64                                             m_fenceValue;
    HANDLE                                             m_fenceEvent;
}; // CommandQueue