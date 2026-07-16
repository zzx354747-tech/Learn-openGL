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
    struct Peak
    {
        float bezierT;
        float amplitude;
        float sigma;
    };

    struct Settings
    {
        unsigned int resolution = 1024;       // height field and TDM
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

        // A basin is placed manually for now; its shoreline is always the
        // height-field/water-plane intersection, never an independent mask.
        glm::vec2 lakeCenter = glm::vec2(-2550.0f, 1650.0f);
        glm::vec2 lakeRadii = glm::vec2(720.0f, 470.0f);
        float lakeBasinDepth = 54.0f;
        float lakeLevelOffset = 8.0f;
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
    unsigned int terrainDataTexture = 0;
    unsigned int detailNoiseTexture = 0;
    float waterLevel = 0.0f;
    bool cacheHit = false;
    std::array<glm::vec4, 6> frustumPlanes{};
    bool streamingValid = false;

    void generate();
    void generateHeightField();
    void computeDerivedFields();
    void buildMesh();
    void uploadTerrainTextures();
    void findWaterLevel();
    std::uint64_t parameterHash() const;
    bool loadCache();
    void saveCache() const;
};
