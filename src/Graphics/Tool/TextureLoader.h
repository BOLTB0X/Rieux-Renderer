#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>

class DescriptorHeapAllocator;

class TextureLoader {
public:
    static bool CreateTextureFromFile(
        ID3D12Device*,
        ID3D12GraphicsCommandList*,
        DescriptorHeapAllocator*,
        const std::string&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        UINT&,
        std::vector<unsigned char>* outPixels = nullptr,
        int* outWidth = nullptr,
        int* outHeight = nullptr
    ); // CreateTextureFromFile

    static bool CreateTextureFromMemory(
        ID3D12Device*,
        ID3D12GraphicsCommandList*,
        DescriptorHeapAllocator*,
        const uint8_t*,
        UINT,
        UINT,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        Microsoft::WRL::ComPtr<ID3D12Resource>&,
        UINT&
    ); // CreateTextureFromMemory
}; // TextureLoader