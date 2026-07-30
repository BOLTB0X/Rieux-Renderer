#include "Pch.h"
#include "AssimpLoader.h"
// Data
#include "Texture.h"
// Components
#include "DescriptorHeapAllocator.h"
// Managers
#include "TextureManager.h"
// Utils
#include "DebugHelper.h"
#include "SharedCommons.h"
// STL
#include <filesystem>

using namespace DebugHelper;
using namespace DirectX;
using namespace Microsoft::WRL;

AssimpLoader::AssimpLoader(std::shared_ptr<TextureManager> texMgr)
    : m_TextureManager(texMgr), m_device(nullptr), m_heapAllocator(nullptr), m_uploadFenceEvent(nullptr), m_uploadFenceValue(0) {
} // AssimpLoader

AssimpLoader::~AssimpLoader() {
    if (m_uploadFenceEvent) {
        CloseHandle(m_uploadFenceEvent);
        m_uploadFenceEvent = nullptr;
    }
    m_heapAllocator = nullptr;
    m_device = nullptr;
} // ~AssimpLoader

bool AssimpLoader::LoadMeshModel(const LoadParams& params) {
    if (!params.device || !params.commandQueue || !params.outModel) {
        DebugPrint("AssimpLoader::LoadMeshModel - 잘못된 파라미터");
        return false;
    }

    m_device = params.device;
    m_heapAllocator = params.heapAllocator;
    m_commandQueue = params.commandQueue;

    if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_uploadAllocator)))) {
        DebugPrint("AssimpLoader 업로드용 CommandAllocator 생성 실패");
        return false;
    }
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_uploadAllocator.Get(), nullptr, IID_PPV_ARGS(&m_uploadCommandList)))) {
        DebugPrint("AssimpLoader 업로드용 CommandList 생성 실패");
        return false;
    }
    m_uploadCommandList->Close();

    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_uploadFence)))) {
        DebugPrint("AssimpLoader 업로드용 Fence 생성 실패");
        return false;
    }
    m_uploadFenceValue = 1;
    m_uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(params.path,
        aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
        aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        DebugPrint(std::string("Assimp 로드 실패: ") + importer.GetErrorString());
        return false;
    }

    std::string directory = std::filesystem::path(params.path).parent_path().string();

    BeginUpload();
    ProcessNode(scene->mRootNode, scene, XMMatrixIdentity(), params.outModel);
    EndUploadAndWait();

    ProcessMaterials(scene, directory, params.outModel);

    return true;
} // LoadMeshModel

void AssimpLoader::BeginUpload() {
    m_uploadAllocator->Reset();
    m_uploadCommandList->Reset(m_uploadAllocator.Get(), nullptr);
    m_pendingUploadBuffers.clear();
} // BeginUpload

void AssimpLoader::EndUploadAndWait() {
    m_uploadCommandList->Close();

    ID3D12CommandList* lists[] = { m_uploadCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(lists), lists);

    const UINT64 fenceValue = m_uploadFenceValue;
    m_commandQueue->Signal(m_uploadFence.Get(), fenceValue);
    m_uploadFenceValue++;

    if (m_uploadFence->GetCompletedValue() < fenceValue) {
        m_uploadFence->SetEventOnCompletion(fenceValue, m_uploadFenceEvent);
        WaitForSingleObject(m_uploadFenceEvent, INFINITE);
    }

    m_pendingUploadBuffers.clear();
} // EndUploadAndWait

void AssimpLoader::ProcessNode(aiNode* node, const aiScene* scene, const XMMATRIX& parentTransform, AssimpModel* model) {
    XMMATRIX nodeTransform = ConvertMatrixToDirectX(node->mTransformation);
    XMMATRIX worldTransform = XMMatrixMultiply(nodeTransform, parentTransform);

    for (UINT i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto pbrMesh = ProcessMesh(mesh, scene, worldTransform, model);
        if (pbrMesh) {
            model->AddMesh(std::move(pbrMesh));
        }
    }

    for (UINT i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, worldTransform, model);
    }
} // ProcessNode

std::unique_ptr<PBRMesh> AssimpLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const XMMATRIX& worldTransform, AssimpModel* model) {
    std::vector<PBRMesh::PBRVertex> vertices;
    vertices.reserve(mesh->mNumVertices);

    XMVECTOR det;
    XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(&det, worldTransform));

    for (UINT i = 0; i < mesh->mNumVertices; ++i) {
        PBRMesh::PBRVertex v;

        // Position
        XMVECTOR pos = XMVectorSet(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        pos = XMVector3Transform(pos, worldTransform);
        XMStoreFloat3(&v.position, pos);

        // Normal
        if (mesh->HasNormals()) {
            XMVECTOR normal = XMVectorSet(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
            normal = XMVector3Normalize(XMVector3TransformNormal(normal, worldInvTranspose));
            XMStoreFloat3(&v.normal, normal);
        }

        // Texture UV
        if (mesh->mTextureCoords[0]) {
            v.texture = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }

        // Tangent & Bitangent
        if (mesh->HasTangentsAndBitangents()) {
            XMVECTOR tangent = XMVectorSet(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f);
            tangent = XMVector3Normalize(XMVector3TransformNormal(tangent, worldInvTranspose));
            XMStoreFloat3(&v.tangent, tangent);

            XMVECTOR bitangent = XMVectorSet(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z, 0.0f);
            bitangent = XMVector3Normalize(XMVector3TransformNormal(bitangent, worldInvTranspose));
            XMStoreFloat3(&v.binormal, bitangent);
        }

        vertices.push_back(v);
    }

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);
    for (UINT i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    auto pbrMesh = std::make_unique<PBRMesh>();

    PBRMesh::InitParams meshParams;
    meshParams.device = m_device;
    meshParams.uploadCmdList = m_uploadCommandList.Get();
    meshParams.vertices = &vertices;
    meshParams.indices = &indices;
    meshParams.materialIndex = mesh->mMaterialIndex;
    meshParams.heapAllocator = m_heapAllocator;
    meshParams.name = mesh->mName.C_Str();

    ComPtr<ID3D12Resource> vbUpload;
    ComPtr<ID3D12Resource> ibUpload;

    if (!pbrMesh->Init(meshParams, vbUpload, ibUpload)) {
        DebugPrint("PBRMesh 생성 실패: " + std::string(mesh->mName.C_Str()));
        return nullptr;
    }

    // fence 대기 끝날 때까지 살려둬야 함
    m_pendingUploadBuffers.push_back(vbUpload);
    m_pendingUploadBuffers.push_back(ibUpload);

    return pbrMesh;
} // ProcessMesh

void AssimpLoader::ProcessMaterials(const aiScene* scene, const std::string& directory, AssimpModel* model) {
    using namespace SharedCommons;

    for (UINT i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* aiMat = scene->mMaterials[i];
        AssimpModel::Material material;

        aiString name;
        aiMat->Get(AI_MATKEY_NAME, name);
        material.name = name.C_Str();

        material.albedo = LoadMaterialElement(aiMat, directory, aiTextureType_DIFFUSE, PBRTextureType::Albedo);
        if (!material.albedo) {
            material.albedo = m_TextureManager->GetTexture(SharedCommons::KEY_DUMMEY_WHITE);
        }
        aiColor4D baseColor(SharedCommons::ALBEDO_FACTOR.x, SharedCommons::ALBEDO_FACTOR.y,
            SharedCommons::ALBEDO_FACTOR.z, SharedCommons::ALBEDO_FACTOR.w);
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        material.albedoFactor = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };

        material.normal = LoadMaterialElement(aiMat, directory, aiTextureType_DISPLACEMENT, PBRTextureType::Normal);
        if (!material.normal) {
            material.normal = LoadMaterialElement(aiMat, directory, aiTextureType_NORMALS, PBRTextureType::Normal);
        }
        if (!material.normal) {
            material.normal = LoadMaterialElement(aiMat, directory, aiTextureType_HEIGHT, PBRTextureType::Normal);
        }
        if (!material.normal) {
            material.normal = m_TextureManager->GetTexture(SharedCommons::KEY_DUMMEY_NORMAL);
        }

        material.alpha = LoadMaterialElement(aiMat, directory, aiTextureType_OPACITY, PBRTextureType::Alpha);
        if (!material.alpha) {
            material.alpha = m_TextureManager->GetTexture(SharedCommons::KEY_DUMMEY_WHITE);
        }

        material.roughness = LoadMaterialElement(aiMat, directory, aiTextureType_DIFFUSE_ROUGHNESS, PBRTextureType::Roughness);
        if (!material.roughness) {
            float rgh = 0.8f;
            if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, rgh) != AI_SUCCESS) {
                material.roughnessFactor = SharedCommons::ROUGH_FACTOR;
            }
            else {
                material.roughnessFactor = rgh;
            }
        }
        else {
            material.roughnessFactor = 1.0f;
        }

        // Metallic (MTL PBR Extension: map_Pm)
        material.metallic = LoadMaterialElement(aiMat, directory, aiTextureType_METALNESS, PBRTextureType::Metallic);
        if (!material.metallic) {
            float met = 0.0f;
            if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, met) != AI_SUCCESS) {
                material.metallicFactor = 0.0f;
            }
            else {
                material.metallicFactor = met;
            }
        }
        else {
            material.metallicFactor = 1.0f;
        }

        material.ao = nullptr;
        material.specular = nullptr;
        material.emissive = nullptr;
        material.displacement = nullptr;
        material.subsurface = nullptr;
        material.smoothness = nullptr;

        model->AddMaterial(material);
    }
} // ProcessMaterials

std::shared_ptr<Texture> AssimpLoader::LoadMaterialElement(
    aiMaterial* material, const std::string& directory, aiTextureType type,
    SharedCommons::PBRTextureType /*pbrType*/) {
    if (material->GetTextureCount(type) == 0) {
        return nullptr;
    }

    aiString texPath;
    if (material->GetTexture(type, 0, &texPath) != AI_SUCCESS) {
        return nullptr;
    }

    std::string fullPath = directory + "/" + texPath.C_Str();
    return m_TextureManager->GetTexture(fullPath);
} // LoadMaterialElement

XMMATRIX AssimpLoader::ConvertMatrixToDirectX(const aiMatrix4x4& m) {
    return XMMatrixSet(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
} // ConvertMatrixToDirectX