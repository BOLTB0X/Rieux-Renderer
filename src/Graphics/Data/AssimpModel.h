#pragma once
#include <d3d12.h>
// STL
#include <vector>
#include <string>
#include <memory>
// Uitls
#include "SharedCommons.h"

class AssimpLoader;
class Texture;
class TextureManager;
class PBRMesh;
class DescriptorHeapAllocator;

class AssimpModel { friend class AssimpLoader;
public:
    struct MaterialInfo {
        std::string name;
        bool hasAlbedo;
        bool hasNormal;
        bool hasMetallic;
        bool hasRoughness;
        bool hasAO;
        bool hasAlpha;
        bool hasSpecular;
        bool hasEmissive;
        bool hasDisplacement;
        bool hasSubsurface;
        bool hasSmoothness;

        MaterialInfo()
            : hasAlbedo(false), hasNormal(false), hasMetallic(false), hasRoughness(false),
            hasAO(false), hasAlpha(false), hasSpecular(false), hasEmissive(false),
            hasDisplacement(false), hasSubsurface(false), hasSmoothness(false) {
        }
    }; // MaterialInfo

    struct InitParams {
        ID3D12Device*                   device;
        ID3D12CommandQueue*             commandQueue;
        std::shared_ptr<TextureManager> textureManager;
        std::string                     path;
        DescriptorHeapAllocator*        heapAllocator;

        InitParams() : device(nullptr), commandQueue(nullptr), textureManager(nullptr), path(""), heapAllocator(nullptr) {
        }
    }; // InitParams

protected:
    struct Material {
        std::string              name;
        std::shared_ptr<Texture> albedo;
        std::shared_ptr<Texture> normal;
        std::shared_ptr<Texture> metallic;
        std::shared_ptr<Texture> roughness;
        std::shared_ptr<Texture> ao;
        std::shared_ptr<Texture> alpha;
        std::shared_ptr<Texture> specular;
        std::shared_ptr<Texture> emissive;
        std::shared_ptr<Texture> displacement;
        std::shared_ptr<Texture> subsurface;
        std::shared_ptr<Texture> smoothness;
        DirectX::XMFLOAT4        albedoFactor;
        float                    metallicFactor;
        float                    roughnessFactor;
        float                    emissiveFactor;

        Material() : name(""), albedoFactor(SharedCommons::ALBEDO_FACTOR), metallicFactor(SharedCommons::METALLIC_FACTOR)
            , roughnessFactor(SharedCommons::ROUGH_FACTOR), emissiveFactor(SharedCommons::EMISS_FACTOR) {
        }
    }; // Material

public:
    AssimpModel();
    virtual ~AssimpModel();

    bool                       Init(const InitParams&);
    int                        GetMeshCount() const;
    std::vector<MaterialInfo>  GetMaterialInfos() const;
    const PBRMesh*             GetMesh(int) const;
    const Material&            GetMaterial(int) const;

protected:
    void AddMesh(std::unique_ptr<PBRMesh>);
    void AddMaterial(const Material&);

protected:
    std::unique_ptr<AssimpLoader>         m_AssimpLoader;
    std::vector<std::unique_ptr<PBRMesh>> m_meshes;
    std::vector<Material>                 m_materials;
}; // AssimpModel