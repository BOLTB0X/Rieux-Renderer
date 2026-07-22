#include "Pch.h"
#include "TextureManager.h"
// Components
#include "DescriptorHeapAllocator.h"
// Resources
#include "Data/Texture.h"
// Utils
#include "DebugHelper.h"
#include "SharedConstants.h"

using namespace DebugHelper;
using namespace SharedConstants;

TextureManager::TextureManager()
    : m_device(nullptr), m_uploadFenceEvent(nullptr), m_uploadFenceValue(0), m_descriptorAllocator(nullptr) {
    m_Textures = std::unordered_map<std::string, std::shared_ptr<Texture>>();
} // TextureManager

TextureManager::~TextureManager() {
    Shutdown();
} // ~TextureManager

bool TextureManager::Init(const InitParams& params) {
    m_device = params.device;
    m_commandQueue = params.commandQueue;
    m_descriptorAllocator = params.sharedDescriptorAllocator;

    if (!m_descriptorAllocator) {
        DebugPrint("TextureManager::Init - sharedDescriptorAllocator가 전달되지 않음");
        return false;
    }

    if (FAILED(params.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_uploadAllocator)))) {
        DebugPrint("업로드용 CommandAllocator 생성 실패");
        return false;
    }

    if (FAILED(params.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_uploadAllocator.Get(), nullptr, IID_PPV_ARGS(&m_uploadCommandList)))) {
        DebugPrint("업로드용 CommandList 생성 실패");
        return false;
    }
    m_uploadCommandList->Close();

    if (FAILED(params.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_uploadFence)))) {
        DebugPrint("업로드용 Fence 생성 실패");
        return false;
    }
    m_uploadFenceValue = 1;
    m_uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    BeginUpload();

    EndUploadAndWait();

    return true;
} // Init

void TextureManager::Shutdown() {
    if (m_uploadFenceEvent) {
        CloseHandle(m_uploadFenceEvent);
        m_uploadFenceEvent = nullptr;
    }
    m_Textures.clear();
} // Shutdown

void TextureManager::BeginUpload() {
    m_uploadAllocator->Reset();
    m_uploadCommandList->Reset(m_uploadAllocator.Get(), nullptr);
    m_pendingUploadBuffers.clear();
} // BeginUpload

void TextureManager::EndUploadAndWait() {
    m_uploadCommandList->Close();

    ID3D12CommandList* lists[] = { m_uploadCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 fenceValue = m_uploadFenceValue;
    m_commandQueue->Signal(m_uploadFence.Get(), fenceValue);
    m_uploadFenceValue++;

    if (m_uploadFence->GetCompletedValue() < fenceValue) {
        m_uploadFence->SetEventOnCompletion(fenceValue, m_uploadFenceEvent);
        WaitForSingleObject(m_uploadFenceEvent, INFINITE);
    }

    // GPU 복사가 끝났으니
    // 중간 업로드 힙은 이제 해제해도 안전함
    m_pendingUploadBuffers.clear();
} // EndUploadAndWait

void TextureManager::LoadTexture(const std::string& filename, bool keepCpuPixels) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_Textures.find(filename);
    if (it != m_Textures.end()) {
        return;
    }

    auto newTexture = std::make_shared<Texture>();
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

    if (newTexture->Init(m_device, m_uploadCommandList.Get(), m_descriptorAllocator,
        filename, uploadBuffer, keepCpuPixels)) {
        m_Textures[filename] = newTexture;
        m_pendingUploadBuffers.push_back(uploadBuffer);
    }
} // LoadTexture

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& filename, bool keepCpuPixels) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_Textures.find(filename);
        if (it != m_Textures.end()) {
            return it->second;
        }
    }

    BeginUpload();
    LoadTexture(filename, keepCpuPixels);
    EndUploadAndWait();

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_Textures.find(filename);
    return (it != m_Textures.end()) ? it->second : nullptr;
} // GetTexture

ID3D12DescriptorHeap* TextureManager::GetSRVHeap() const {
    return m_descriptorAllocator->GetHeap();
} // GetSRVHeap