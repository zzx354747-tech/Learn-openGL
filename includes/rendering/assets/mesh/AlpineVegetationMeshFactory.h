#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

struct VegetationVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 colorRoughness;
    glm::vec2 windVariation;
};

struct VegetationMeshData
{
    std::vector<VegetationVertex> vertices;
    std::vector<std::uint16_t> indices;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);

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
    static VegetationMeshLODSet makeCushionPlantA(std::uint32_t seed);
    static VegetationMeshLODSet makeCushionPlantB(std::uint32_t seed);
};
