#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>

class DescriptorHeapAllocator;

class Texture {
public:
    Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture();

    bool Init(ID3D12Device*, ID3D12GraphicsCommandList*,
        DescriptorHeapAllocator*, const std::string&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&, bool keepCpuPixels = false);

    UINT                      GetSRVIndex() const; // Dynamic Indexing 단계에서 셰이더로 넘길 값
    ID3D12Resource*           GetResource() const;
    float                     GetPixelHeight(int, int) const;
    int                       GetWidth() const;
    int                       GetHeight() const;

    void                      SetFromGPU(ID3D12Resource*, UINT srvIndex, int, int, const std::vector<float>&);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    UINT                                   m_srvIndex;
    std::vector<unsigned char>             m_cpuPixels;
    std::vector<float>                     m_cpuHeightPixels;
    int                                    m_width;
    int                                    m_height;
}; // Texture