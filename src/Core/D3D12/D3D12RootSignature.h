#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <deque>
#include <vector>
#include <unordered_map>

class D3D12RootSignature {
public:
    class Builder {
    public:
        Builder& AddCBV(const std::string&, UINT, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        Builder& AddConstants(const std::string&, UINT, UINT, D3D12_SHADER_VISIBILITY);
        Builder& AddSRV(const std::string&, UINT, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        Builder& AddSRVTable(const std::string&, UINT, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL, UINT numDescriptors = 1, UINT registerSpace = 0);
        Builder& AddUAVTable(const std::string&, UINT, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT numDescriptors = 1, UINT registerSpace = 0);
        Builder& AddStaticSampler(UINT,
            D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL);

        Builder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS);

    private:
        friend class D3D12RootSignature;

        std::vector<D3D12_ROOT_PARAMETER>       m_rootParameters;
        std::deque<D3D12_DESCRIPTOR_RANGE>      m_descriptorRanges;
        std::vector<D3D12_STATIC_SAMPLER_DESC>  m_staticSamplers;
        std::unordered_map<std::string, UINT>   m_paramIndexByTag;
        D3D12_ROOT_SIGNATURE_FLAGS              m_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }; // Builder

    struct InitParams {
        ID3D12Device* device = nullptr;
        Builder       builder;
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
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    std::unordered_map<std::string, UINT>       m_paramIndexMap;
}; // D3D12RootSignature