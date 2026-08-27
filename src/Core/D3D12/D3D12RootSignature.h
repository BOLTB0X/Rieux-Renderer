#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <deque>
#include <vector>
#include <unordered_map>
#include <string>
// Tools
#include "RootSignatureBuilder.h"

class D3D12RootSignature {
public:
    struct InitParams {
        ID3D12Device*        device = nullptr;
        RootSignatureBuilder builder;
        //Builder       builder;
    }; // InitDefaultParams

public:
    D3D12RootSignature() = default;
    D3D12RootSignature(const D3D12RootSignature&) = delete;
    D3D12RootSignature& operator=(const D3D12RootSignature&) = delete;
    ~D3D12RootSignature() = default;

    bool Init(const InitParams&);
    ID3D12RootSignature* GetRootSignature() const;
    UINT                 GetParamIndex(const std::string&) const;

private:
    friend class RootSignatureBuilder;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    std::unordered_map<std::string, UINT>       m_paramIndexMap;
}; // D3D12RootSignature