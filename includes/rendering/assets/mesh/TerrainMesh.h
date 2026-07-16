#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "rendering/assets/texture/Material.h"
#include "rendering/modelload/Mesh.h"

// Startup-generated Alpine terrain. Height and all derived data are immutable
// after construction; every runtime consumer reads the Terrain Data Map (TDM).
class TerrainMesh
{
public:
    struct LakeRegion
    {
        glm::vec4 boundsXZ = glm::vec4(0.0f); // minX, minZ, maxX, maxZ
        float waterLevel = 0.0f;
        float area = 0.0f;
        float maximumDepth = 0.0f;
    };

    struct Peak
    {
        float bezierT;
        float amplitude;
        float sigma;
    };

    struct Settings
    {
        unsigned int resolution = 1024;       // height field and TDM
        unsigned int lakeDataResolution = 2048; // analytic shoreline/LDM
        unsigned int meshResolution = 256;    // base mesh (not height data)
        float size = 8200.0f;
        float mountainHeight = 2050.0f;
        float baseHeight = 30.0f;
        unsigned int seed = 1337;

        glm::vec2 ridgeP0 = glm::vec2(-1850.0f, -820.0f);
        glm::vec2 ridgeP1 = glm::vec2(100.0f, 980.0f);
        glm::vec2 ridgeP2 = glm::vec2(1850.0f, -640.0f);
        std::array<Peak, 3> peaks{{
            {0.12f, 0.88f, 920.0f},
            {0.50f, 1.00f, 1080.0f},
            {0.84f, 0.82f, 820.0f}}};
        float smoothMaxK = 0.30f;
        float baseContribution = 0.66f;
        int ridgedOctaves = 6;
        float ridgedContribution = 0.31f;
        float ridgedGain = 2.0f;
        float lacunarity = 2.0f;
        float persistence = 0.5f;
        float detailContribution = 0.03f;
        float warpStrength = 150.0f;
        float upliftExponent = 1.30f;
        bool edgeFade = true;
        float curvatureRange = 0.012f;

        // Lake terrain is part of the authored terrain preset. The same
        // deterministic shape function carves the floor, builds the bank and
        // writes the Lake Data Map; there is no minimum search or flood fill.
        glm::vec2 lakeCenter = glm::vec2(0.0f, 0.0f);
        glm::vec2 lakeRadii = glm::vec2(460.0f, 310.0f);
        float lakeWaterLevel = 1167.0f;
        float lakeBasinDepth = 68.0f;
        float lakeRimHeight = 24.0f;
        float lakeBankWidth = 120.0f;

        // A separate plateau tarn beside the meadow, designed independently
        // but evaluated by the same terrain/lake function.
        glm::vec2 meadowLakeCenter = glm::vec2(-2200.0f, 1300.0f);
        glm::vec2 meadowLakeRadii = glm::vec2(320.0f, 220.0f);
        float meadowLakeWaterLevel = 72.6f;
        float meadowLakeBasinDepth = 28.0f;
        float meadowLakeRimHeight = 14.0f;
        float meadowLakeBankWidth = 82.0f;
    };

    TerrainMesh();
    explicit TerrainMesh(const Settings& settings);
    ~TerrainMesh();

    TerrainMesh(const TerrainMesh&) = delete;
    TerrainMesh& operator=(const TerrainMesh&) = delete;

    void draw(Shader& shader) const;
    void updateStreaming(const glm::vec3& cameraPosition,
                         const glm::mat4& viewProjection);
    float sampleHeight(float worldX, float worldZ) const;
    glm::vec3 sampleNormal(float worldX, float worldZ) const;
    float sampleWorldHeight(float worldX, float worldZ) const;
    glm::vec3 sampleWorldNormal(float worldX, float worldZ) const;
    bool isBelowWater(float worldX, float worldZ) const;
    glm::vec2 sampleLakeData(float worldX, float worldZ) const;
    float sampleWaterDepth(float worldX, float worldZ) const;
    float sampleSignedDistanceToWater(float worldX, float worldZ) const;
    bool isInsideLake(float worldX, float worldZ) const;
    bool isInsideCentralTerrain(float worldX, float worldZ) const;
    glm::ivec2 getStreamingTile() const { return glm::ivec2(0); }
    glm::mat4 getTileTransform(int, int) const { return glm::mat4(1.0f); }
    bool isTileVisible(int tileX, int tileZ) const;
    float tileDistance(int, int) const { return 0.0f; }
    bool useFullTerrainDetail(int, int) const { return true; }
    static constexpr int StreamingTileRadius = 0;

    const Settings& getSettings() const { return settings; }
    float getWaterLevel() const { return waterLevel; }
    unsigned int getTerrainDataTexture() const { return terrainDataTexture; }
    unsigned int getLakeDataTexture() const { return lakeDataTexture; }
    glm::vec4 getLakeBoundsXZ() const { return lakeBoundsXZ; }
    float getLakeArea() const { return lakeArea; }
    float getMaximumWaterDepth() const { return maximumWaterDepth; }
    const std::vector<LakeRegion>& getLakeRegions() const { return lakeRegions; }
    bool wasLoadedFromCache() const { return cacheHit; }

private:
    Settings settings;
    Material rockMaterial;
    Material grassMaterial;
    Material snowMaterial;
    std::unique_ptr<Mesh> mesh;
    std::vector<float> heightSamples;
    std::vector<float> terrainData; // transient derived-field workspace
    std::vector<std::uint16_t> terrainDataHalf;
    // RGB: depth, signed shore distance, local water-surface elevation.
    std::vector<std::uint16_t> lakeDataHalf;
    std::vector<LakeRegion> lakeRegions;
    unsigned int terrainDataTexture = 0;
    unsigned int lakeDataTexture = 0;
    unsigned int detailNoiseTexture = 0;
    float waterLevel = 0.0f;
    glm::vec4 lakeBoundsXZ = glm::vec4(0.0f);
    float lakeArea = 0.0f;
    float maximumWaterDepth = 0.0f;
    bool cacheHit = false;
    std::array<glm::vec4, 6> frustumPlanes{};
    bool streamingValid = false;

    void generate();
    void generateHeightField();
    void computeDerivedFields();
    void buildMesh();
    void uploadTerrainTextures();
    void generateLakeData();
    std::uint64_t parameterHash() const;
    bool loadCache();
    void saveCache() const;
};
