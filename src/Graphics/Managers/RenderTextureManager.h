#pragma once
#include <d3d12.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "D3D12/RenderTexture.h"

class DescriptorHeapAllocator;

class RenderTextureManager {
public:
    struct InitParams {
        ID3D12Device*            device;
        DescriptorHeapAllocator* sharedDescriptorAllocator;
        UINT                     rtvCapacity;
        UINT                     dsvCapacity;

        InitParams() : device(nullptr), sharedDescriptorAllocator(nullptr), rtvCapacity(64), dsvCapacity(16) {
        }
    }; // InitParams

public:
    RenderTextureManager();
    RenderTextureManager(const RenderTextureManager&) = delete;
    RenderTextureManager& operator=(const RenderTextureManager&) = delete;
    ~RenderTextureManager();

    bool Init(const InitParams&);

public:
    std::shared_ptr<RenderTexture> CreateRenderTexture(const std::string&, UINT, UINT,
        RenderTexture::RenderTextureType type, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT);
    std::shared_ptr<RenderTexture> GetRenderTexture(const std::string&) const;

private:
    ID3D12Device*                                                   m_device;
    DescriptorHeapAllocator*                                        m_sharedDescriptorAllocator;
    std::unique_ptr<DescriptorHeapAllocator>                        m_rtvAllocator;
    std::unique_ptr<DescriptorHeapAllocator>                        m_dsvAllocator;
    std::unordered_map<std::string, std::shared_ptr<RenderTexture>> m_renderTextures;
    std::mutex                                                      m_mutex;
}; // RenderTextureManager