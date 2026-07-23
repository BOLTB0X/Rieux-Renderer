#pragma once
#include <d3d12.h>
#include <wrl/client.h>
// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
// STL
#include <string>
#include <memory>
#include <vector>
// Resources
#include "Data/AssimpModel.h"
#include "Data/PBRMesh.h"
// Utils
#include "SharedCommons.h"

class Texture;
class TextureManager;

class AssimpLoader {
public:
    struct LoadParams {
        ID3D12Device* device;
        ID3D12CommandQueue* commandQueue;
        std::string         path;
        AssimpModel* outModel;

        LoadParams() : device(nullptr), commandQueue(nullptr), path(""), outModel(nullptr) {
        }
    }; // LoadParams

public:
    AssimpLoader(std::shared_ptr<TextureManager>);
    AssimpLoader(const AssimpLoader&) = delete;
    AssimpLoader& operator=(const AssimpLoader&) = delete;
    ~AssimpLoader();

    bool LoadMeshModel(const LoadParams&);

private:
    void                     ProcessNode(aiNode*, const aiScene*, const DirectX::XMMATRIX&, AssimpModel*);
    std::unique_ptr<PBRMesh> ProcessMesh(aiMesh*, const aiScene*, const DirectX::XMMATRIX&, AssimpModel*);
    void                     ProcessMaterials(const aiScene*, const std::string&, AssimpModel*);
    std::shared_ptr<Texture> LoadMaterialElement(aiMaterial*, const std::string&, aiTextureType, SharedCommons::PBRTextureType);
    DirectX::XMMATRIX        ConvertMatrixToDirectX(const aiMatrix4x4&);
    void                     BeginUpload();
    void                     EndUploadAndWait();

private:
    std::shared_ptr<TextureManager>                     m_TextureManager;
    ID3D12Device*                                       m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_uploadAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_uploadCommandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_uploadFence;
    HANDLE                                              m_uploadFenceEvent;
    UINT64                                              m_uploadFenceValue;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_pendingUploadBuffers;
}; // AssimpLoader