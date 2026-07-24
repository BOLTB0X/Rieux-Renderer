#pragma once
#include <d3d12.h>
#include <wrl.h>

class D3D12RootSignature {
public:
    struct InitParams {
        ID3D12Device* device;

        InitParams() : device(nullptr) {}
    }; // InitParams

public:
    D3D12RootSignature() = default;
    D3D12RootSignature(const D3D12RootSignature&) = delete;
    D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;
    ~D3D12RootSignature() = default;

    bool                 Init(const InitParams&);
    ID3D12RootSignature* GetRootSignature() const;

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
}; // D3D12RootSignature