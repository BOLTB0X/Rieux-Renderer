#include "Pch.h"
#include "Texture.h"
#include "Tool/TextureLoader.h"
#include "Components/DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"
// STL
#include <algorithm>

Texture::Texture() {
    m_srvIndex = UINT_MAX;
    m_width = 0;
    m_height = 0;
}; // Texture

Texture::~Texture() {
}; // ~Texture

bool Texture::Init(ID3D12Device* device, ID3D12GraphicsCommandList* uploadCmdList,
    DescriptorHeapAllocator* descriptorAllocator, const std::string& path,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadBuffer, bool keepCpuPixels) {

    if (keepCpuPixels) {
        return TextureLoader::CreateTextureFromFile(
            device, uploadCmdList, descriptorAllocator, path,
            m_resource, outUploadBuffer, m_srvIndex,
            &m_cpuPixels, &m_width, &m_height);
    }

    return TextureLoader::CreateTextureFromFile(
        device, uploadCmdList, descriptorAllocator, path,
        m_resource, outUploadBuffer, m_srvIndex);
} // Init

UINT Texture::GetSRVIndex() const {
    return m_srvIndex;
} // GetSRVIndex

ID3D12Resource* Texture::GetResource() const {
    return m_resource.Get();
} // GetResource

float Texture::GetPixelHeight(int x, int y) const {
    if (!m_cpuHeightPixels.empty()) {
        x = std::max(0, std::min(x, m_width - 1));
        y = std::max(0, std::min(y, m_height - 1));

        return m_cpuHeightPixels[y * m_width + x];
    }

    if (!m_cpuPixels.empty()) {
        x = std::max(0, std::min(x, m_width - 1));
        y = std::max(0, std::min(y, m_height - 1));

        int i = ((y * m_width) + x) * 4;
        return static_cast<float>(m_cpuPixels[i]) / 255.0f;
    }

    return 0.0f;
} // GetPixelHeight

int Texture::GetWidth() const {
    return m_width;
} // GetWidth

int Texture::GetHeight() const {
    return m_height;
} // GetHeight

void Texture::SetFromGPU(ID3D12Resource* resource, UINT srvIndex, int w, int h, const std::vector<float>& cpuData) {
    m_resource = resource;
    m_srvIndex = srvIndex;
    m_width = w;
    m_height = h;
    m_cpuHeightPixels = cpuData;
} // SetFromGPU