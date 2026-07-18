#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

struct VegetationVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 colorRoughness;
    glm::vec2 windVariation;
    // Textured meshes: xy = UV0, z/w = base-colour/normal texture weights.
    // Procedural meshes leave this payload at zero.
    glm::vec4 uvMaterial = glm::vec4(0.0f);
};

struct VegetationImageData
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    bool empty() const { return width <= 0 || height <= 0 || rgba.empty(); }
};

struct VegetationMaterialSlot
{
    glm::vec4 tileRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    float alphaCutoff = 0.5f;
    bool alphaMask = false;
    bool doubleSided = false;
};

struct VegetationMeshData
{
    std::vector<VegetationVertex> vertices;
    std::vector<std::uint32_t> indices;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    std::vector<VegetationImageData> baseColorTextures;
    std::vector<VegetationImageData> normalTextures;
    // Optional explicit material-atlas mip chains. Procedural vegetation
    // leaves these empty and uses per-vertex colour/roughness instead.
    std::vector<VegetationImageData> baseColorAtlasMips;
    std::vector<VegetationImageData> normalAtlasMips;
    std::vector<VegetationImageData> foliageDataAtlasMips;
    std::array<VegetationMaterialSlot, 4> materialSlots{};
    int materialCount = 0;
    float alphaCutoff = 0.5f;
    bool alphaMask = false;
    bool doubleSided = false;

    bool hasRuntimeMaterialAtlas() const
    {
        return materialCount > 0 && !baseColorAtlasMips.empty() &&
               !normalAtlasMips.empty() && !foliageDataAtlasMips.empty();
    }

    void updateBounds();
};

struct VegetationMeshLODSet
{
    VegetationMeshData lod0;
    VegetationMeshData lod1;
    VegetationMeshData lod2;
    VegetationMeshData shadow;
};

class AlpineVegetationMeshFactory
{
public:
    static VegetationMeshLODSet makeTallConifer(std::uint32_t seed);
    static VegetationMeshLODSet makeBroadConifer(std::uint32_t seed);
    static VegetationMeshLODSet makeSapling(std::uint32_t seed);
    static VegetationMeshLODSet makeRoundShrub(std::uint32_t seed);
    static VegetationMeshLODSet makeWindSweptShrub(std::uint32_t seed);
    static VegetationMeshLODSet makeGrassTuftA(std::uint32_t seed);
    static VegetationMeshLODSet makeGrassTuftB(std::uint32_t seed);
    static VegetationMeshLODSet makeGrassTuftC(std::uint32_t seed);
    static VegetationMeshLODSet makeStarFlower(std::uint32_t seed);
    static VegetationMeshLODSet makeBellFlower(std::uint32_t seed);
    static VegetationMeshLODSet makeSpikeFlower(std::uint32_t seed);
    static VegetationMeshLODSet makePinkFlower(std::uint32_t seed);
    static VegetationMeshLODSet makeCrocusFlower(std::uint32_t seed);
    static VegetationMeshLODSet makeCushionPlantA(std::uint32_t seed);
    static VegetationMeshLODSet makeCushionPlantB(std::uint32_t seed);
};
