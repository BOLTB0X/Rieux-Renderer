#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

class Texture;
class DescriptorHeapAllocator;

class TextureManager {
public:
    struct InitParams {
        ID3D12Device*            device;
        ID3D12CommandQueue*      commandQueue;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        HWND                     hwnd;

        InitParams() : device(nullptr), commandQueue(nullptr), sharedDescriptorAllocator(nullptr), hwnd(nullptr) {
        }
    }; // InitParams

public:
    TextureManager();
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    ~TextureManager();

    bool Init(const InitParams&);
    void Shutdown();

    std::shared_ptr<Texture> GetTexture(const std::string&, bool keepCpuPixels = false);
    ID3D12DescriptorHeap* GetSRVHeap() const;

private:
    void LoadTexture(const std::string&, bool keepCpuPixels = false);
    void BeginUpload();
    void EndUploadAndWait();

private:
    ID3D12Device*                                              m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>                 m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>             m_uploadAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>          m_uploadCommandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                        m_uploadFence;
    HANDLE                                                     m_uploadFenceEvent;
    UINT64                                                     m_uploadFenceValue;

    DescriptorHeapAllocator*                                   m_descriptorAllocator;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>        m_pendingUploadBuffers;

    std::unordered_map<std::string, std::shared_ptr<Texture>>  m_Textures;
    std::mutex                                                 m_mutex;
}; // TextureManager