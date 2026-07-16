#pragma once

#include <memory>
#include <array>
#include <vector>

#include "rendering/modelload/Mesh.h"
#include "rendering/assets/texture/Material.h"

// A deterministic, CPU-generated height field inspired by Mount Fuji.
// The resulting mesh uses the same vertex/material layout as imported models,
// so it participates in deferred rendering, forward rendering and shadows.
class TerrainMesh
{
public:
    struct Settings
    {
        // Fuji keeps its original footprint while a second, broad lowland ring
        // expands the playable landscape well beyond the mountain foot.
        unsigned int resolution = 1025;
        float size = 8200.0f;
        float mountainHeight = 2050.0f;
        float baseHeight = 30.0f;
        unsigned int seed = 1337;
    };

    TerrainMesh();
    explicit TerrainMesh(const Settings& settings);
    ~TerrainMesh();

    TerrainMesh(const TerrainMesh&) = delete;
    TerrainMesh& operator=(const TerrainMesh&) = delete;

    void draw(Shader& shader) const;
    void updateStreaming(
        const glm::vec3& cameraPosition,
        const glm::mat4& viewProjection);
    float sampleHeight(float worldX, float worldZ) const;
    glm::vec3 sampleNormal(float worldX, float worldZ) const;
    float sampleWorldHeight(float worldX, float worldZ) const;
    glm::vec3 sampleWorldNormal(float worldX, float worldZ) const;
    bool isBelowWater(float worldX, float worldZ) const;
    static float sampleWaterMask(float worldX, float worldZ);
    bool isInsideCentralTerrain(float worldX, float worldZ) const;
    glm::ivec2 getStreamingTile() const { return streamingTile; }
    glm::mat4 getTileTransform(int tileX, int tileZ) const;
    bool isTileVisible(int tileX, int tileZ) const;
    float tileDistance(int tileX, int tileZ) const;
    bool useFullTerrainDetail(int tileX, int tileZ) const;
    bool useFullVegetationDetail(int tileX, int tileZ) const;
    static constexpr int StreamingTileRadius = 0;
    static constexpr float WaterLevel = -3.0f;
    static constexpr float FujiCenterX = 0.0f;
    static constexpr float FujiCenterZ = -300.0f;
    static constexpr float FujiRadius = 2050.0f;
    static constexpr float LakeCenterX = FujiCenterX;
    static constexpr float LakeCenterZ = FujiCenterZ;
    static constexpr float LakeRadiusX = 2200.0f;
    static constexpr float LakeRadiusZ = 210.0f;
    static constexpr float GrassLine = 620.0f;
    static constexpr float TreeLine = 720.0f;
    static constexpr float SnowLine = 1260.0f;
    const Settings& getSettings() const { return settings; }

private:
    Settings settings;
    Material terrainMaterial;
    Material grassMaterial;
    Material snowMaterial;
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Mesh> lodMesh;
    std::vector<float> heightSamples;
    unsigned int blendMaskTexture = 0;
    glm::ivec2 streamingTile = glm::ivec2(0);
    glm::vec3 streamingCameraPosition = glm::vec3(0.0f);
    std::array<glm::vec4, 6> frustumPlanes{};
    bool streamingValid = false;

    void generate();
    void generateLodMesh();
    glm::vec2 mapWorldToCentral(float worldX, float worldZ) const;
    float sampleGrassWeight(
        float worldX,
        float worldZ,
        float height,
        float normalY) const;
    float sampleSnowWeight(
        float worldX,
        float worldZ,
        float height,
        float normalY) const;
};
