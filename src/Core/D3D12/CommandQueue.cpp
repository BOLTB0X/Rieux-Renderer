#include "Pch.h"
#include "CommandQueue.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

CommandQueue::CommandQueue() : m_fenceValue(0), m_fenceEvent(nullptr) {
} // CommandQueue

CommandQueue::~CommandQueue() {
} // ~CommandQueue

bool CommandQueue::Init(ID3D12Device* device) {
    if (!device) {
        DebugHelper::DebugPrint("CommandQueue 초기화 실패: 디바이스가 유효하지 않습니다.");
        return false;
    }

    // 커맨드 큐 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
        DebugHelper::DebugPrint("ID3D12CommandQueue 생성 실패");
        return false;
    }

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) {
        DebugHelper::DebugPrint("ID3D12CommandAllocator 생성 실패");
        return false;
    }

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
        DebugHelper::DebugPrint("ID3D12GraphicsCommandList 생성 실패");
        return false;
    }
    m_commandList->Close();

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
        DebugHelper::DebugPrint("ID3D12Fence 생성 실패");
        return false;
    }
    m_fenceValue = 1;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) {
        DebugHelper::DebugPrint("Fence 이벤트 생성 실패");
        return false;
    }

    return true;
} // Init

void CommandQueue::Shutdown() {
    WaitForPreviousFrame();

    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    if (m_commandList) {
        m_commandList.Reset();
    }
    if (m_commandAllocator) {
        m_commandAllocator.Reset();
    }
    if (m_commandQueue) {
        m_commandQueue.Reset();
    }
    if (m_fence) {
        m_fence.Reset();
    }
} // Shutdown

bool CommandQueue::Reset() {
    if (!m_commandAllocator || !m_commandList) {
        return false;
    }

    HRESULT hr = m_commandAllocator->Reset();
    if (FAILED(hr)) {
        DebugHelper::DebugPrint("Command allocator Reset 실패");
        return false;
    }

    hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    if (FAILED(hr)) {
        DebugHelper::DebugPrint("Command list Reset 실패");
        return false;
    }

    return true;
} // Reset

void CommandQueue::Execute() {
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
} // Execute

void CommandQueue::WaitForPreviousFrame() {
    const UINT64 fence = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fence);
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
} // WaitForPreviousFrame

ID3D12CommandQueue*        CommandQueue::GetQueue() const { return m_commandQueue.Get(); }
ID3D12GraphicsCommandList* CommandQueue::GetList() const { return m_commandList.Get(); }
ID3D12CommandAllocator*    CommandQueue::GetAllocator() const { return m_commandAllocator.Get(); }