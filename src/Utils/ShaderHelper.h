#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>

namespace ShaderHelper {
    inline static std::wstring entryPoint = L"main";
    inline static std::wstring vsProfile = L"vs_6_0";
    inline static std::wstring psProfile = L"ps_6_0";
    inline static std::wstring csProfile = L"cs_6_0";
    inline static std::wstring gsProfile = L"gs_6_0";
    inline static std::wstring hsProfile = L"hs_6_0";
    inline static std::wstring dsProfile = L"ds_6_0";

    bool CompileShader(const std::wstring& path, const std::wstring& entryPoint, const std::wstring& profile, IDxcBlob** outBlob);

    bool InitVertexShader(const std::wstring& path, IDxcBlob** outBlob);
    bool InitPixelShader(const std::wstring& path, IDxcBlob** outBlob);
    bool InitComputeShader(const std::wstring& path, IDxcBlob** outBlob);
    bool InitGeometryShader(const std::wstring& path, IDxcBlob** outBlob);
    bool InitHullShader(const std::wstring& path, IDxcBlob** outBlob);
    bool InitDomainShader(const std::wstring& path, IDxcBlob** outBlob);

    template<typename T>
    bool InitConstantBuffer(ID3D12Device* device, ID3D12Resource** outBuffer) {
        // DX12의 상수 버퍼는 반드시 256바이트 정렬(Alignment)이 되어야 함
        UINT size = (sizeof(T) + 255) & ~255;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPU에서 쓰고 GPU가 읽는 용도

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = size;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        return SUCCEEDED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(outBuffer)
        ));
    } // InitConstantBuffer

    template<typename T>
    bool UpdateConstantBuffer(ID3D12Resource* buffer, const T& data) {
        void* pMappedData = nullptr;
        D3D12_RANGE readRange = { 0, 0 }; // CPU에서 읽지 않음 명시

        if (FAILED(buffer->Map(0, &readRange, &pMappedData))) {
            return false;
        }
        memcpy(pMappedData, &data, sizeof(T));
        buffer->Unmap(0, nullptr);
        return true;
    } // UpdateConstantBuffer
} // ShaderHelper