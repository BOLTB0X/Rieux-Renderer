#include "Pch.h"
#include "RootSignatureBuilder.h"
// Core
#include "RendererState.h"
// Utils
#include "DebugHelper.h"

using namespace Microsoft::WRL;

RootSignatureBuilder& RootSignatureBuilder::AddCBV(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddCBV

RootSignatureBuilder& RootSignatureBuilder::AddConstants(const std::string& tag,
    UINT shaderRegister, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.Num32BitValues = num32BitValues;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddConstants

RootSignatureBuilder& RootSignatureBuilder::AddSRV(const std::string& tag,
    UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddSRV

RootSignatureBuilder& RootSignatureBuilder::AddSRVTable(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT numDescriptors, UINT registerSpace) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_DESCRIPTOR_RANGE range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numDescriptors, shaderRegister, registerSpace,
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
    };
    m_descriptorRanges.push_back(range);

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &m_descriptorRanges.back();
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddSRVTable

RootSignatureBuilder& RootSignatureBuilder::AddUAVTable(const std::string& tag,
    UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT numDescriptors, UINT registerSpace) {
    m_paramIndexByTag[tag] = static_cast<UINT>(m_rootParameters.size());

    D3D12_DESCRIPTOR_RANGE range = {
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numDescriptors, shaderRegister, registerSpace,
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
    };
    m_descriptorRanges.push_back(range);

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = &m_descriptorRanges.back();
    param.ShaderVisibility = visibility;

    m_rootParameters.push_back(param);
    return *this;
} // AddUAVTable

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(
    UINT shaderRegister, D3D12_FILTER filter,
    D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_SHADER_VISIBILITY visibility) {
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = filter;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = addressMode;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = shaderRegister;
    sampler.ShaderVisibility = visibility;
    m_staticSamplers.push_back(sampler);

    return *this;
} // AddStaticSampler

RootSignatureBuilder& RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    m_flags = flags;
    return *this;
} // SetFlags