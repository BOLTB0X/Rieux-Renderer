#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

class D3D12Device {
public:
    D3D12Device() = default;
    D3D12Device(const D3D12Device&) = delete;
    D3D12Device& operator=(const D3D12Device&) = delete;
    ~D3D12Device() = default;

    bool           Init();
    ID3D12Device*  GetDevice() const;
    IDXGIAdapter3* GetAdapter() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Device>  m_device;
    Microsoft::WRL::ComPtr<IDXGIAdapter3> m_iDXGIAdapter3;
}; // D3D12Device