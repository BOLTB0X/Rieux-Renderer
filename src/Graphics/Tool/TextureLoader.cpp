#include "Pch.h"
#include "TextureLoader.h"
// Core
#include "RendererState.h"
// Components
#include "DescriptorHeapAllocator.h"
// Utils
#include "DebugHelper.h"
// STL
#include <filesystem>
#include <algorithm>
// DirectXTex
#include <DirectXTex.h>

using namespace DebugHelper;

bool TextureLoader::CreateTextureFromFile(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* uploadCmdList,
    DescriptorHeapAllocator* descriptorAllocator,
    const std::string& filename,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outResource,
    Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadBuffer,
    UINT& outSRVIndex,
    std::vector<unsigned char>* outPixels,
    int* outWidth,
    int* outHeight) {

    HRESULT hr = S_OK;
    DirectX::ScratchImage image;

    std::wstring wFilename(filename.begin(), filename.end());
    std::string ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".dds") {
        hr = DirectX::LoadFromDDSFile(wFilename.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
    }
    else if (ext == ".tga") {
        hr = DirectX::LoadFromTGAFile(wFilename.c_str(), nullptr, image);
    }
    else {
        hr = DirectX::LoadFromWICFile(wFilename.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
    }

    if (FAILED(hr)) {
        DebugPrint(filename + " 로드 실패");
        return false;
    }

    if (outPixels && outWidth && outHeight) {
        *outWidth = static_cast<int>(image.GetMetadata().width);
        *outHeight = static_cast<int>(image.GetMetadata().height);

        const DirectX::Image* img = nullptr;
        DirectX::ScratchImage convertedImage;

        if (image.GetMetadata().format != RendererState::RTVFormat) {
            hr = DirectX::Convert(
                image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                RendererState::RTVFormat, DirectX::TEX_FILTER_DEFAULT,
                DirectX::TEX_THRESHOLD_DEFAULT, convertedImage);

            if (SUCCEEDED(hr)) {
                img = convertedImage.GetImage(0, 0, 0);
            }
        }
        else {
            img = image.GetImage(0, 0, 0);
        }

        if (img) {
            size_t pureRowBytes = (*outWidth) * 4;
            outPixels->resize((*outHeight) * pureRowBytes);

            for (int y = 0; y < *outHeight; ++y) {
                uint8_t* dest = outPixels->data() + (y * pureRowBytes);
                const uint8_t* src = img->pixels + (y * img->rowPitch);
                memcpy(dest, src, pureRowBytes);
            }
        }
    }

    // 밉맵 없으면 CPU 측에서 생성
    if (image.GetMetadata().mipLevels == 1) {
        DirectX::ScratchImage mipChain;
        hr = DirectX::GenerateMipMaps(
            image.GetImages(), image.GetImageCount(), image.GetMetadata(),
            DirectX::TEX_FILTER_DEFAULT, 0, mipChain);

        if (SUCCEEDED(hr)) {
            image = std::move(mipChain);
        }
    }

    const auto& metadata = image.GetMetadata();

    // 최종 텍스처가 상주할 default heap 리소스
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = metadata.width;
    texDesc.Height = static_cast<UINT>(metadata.height);
    texDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    texDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    texDesc.Format = metadata.format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    hr = device->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&outResource));

    if (FAILED(hr)) {
        DebugPrint(filename + " 텍스처 리소스 생성 실패");
        return false;
    }

    // CPU->GPU 중간 업로드 버퍼
    const UINT subresourceCount = static_cast<UINT>(metadata.mipLevels * metadata.arraySize);
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(outResource.Get(), 0, subresourceCount);

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    hr = device->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outUploadBuffer));

    if (FAILED(hr)) {
        DebugPrint(filename + " 업로드 힙 생성 실패");
        return false;
    }

    // 서브리소스별 데이터를 커맨드리스트에 복사 명령으로 기록
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    subresources.reserve(subresourceCount);

    for (size_t item = 0; item < metadata.arraySize; ++item) {
        for (size_t mip = 0; mip < metadata.mipLevels; ++mip) {
            const DirectX::Image* mipImage = image.GetImage(mip, item, 0);
            if (!mipImage) {
                continue;
            }

            D3D12_SUBRESOURCE_DATA subData = {};
            subData.pData = mipImage->pixels;
            subData.RowPitch = static_cast<LONG_PTR>(mipImage->rowPitch);
            subData.SlicePitch = static_cast<LONG_PTR>(mipImage->slicePitch);
            subresources.push_back(subData);
        }
    }

    UpdateSubresources(
        uploadCmdList, outResource.Get(), outUploadBuffer.Get(),
        0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    // 복사 완료 후 픽셀 셰이더에서 읽을 수 있는 상태로 전환
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        outResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    uploadCmdList->ResourceBarrier(1, &barrier);

    // 힙에 SRV 등록, 인덱스 반환
    outSRVIndex = descriptorAllocator->Allocate();
    if (outSRVIndex == UINT_MAX) {
        DebugPrint(filename + " 디스크립터 할당 실패 (힙 용량 초과)");
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

    device->CreateShaderResourceView(outResource.Get(), &srvDesc, descriptorAllocator->GetCPUHandle(outSRVIndex));

    return true;
} // CreateTextureFromFile