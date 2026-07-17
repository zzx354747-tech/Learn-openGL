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

    // Stable CPU contract for systems that must attach content to the exact
    // rendered terrain. Vegetation consumes this atomically so height, normal,
    // biome classification and hydrology can never come from different paths.
    struct SurfaceSample
    {
        glm::vec3 worldPosition = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec4 tdm = glm::vec4(0.0f, 0.0f, 0.5f, 0.5f);
        float signedDistanceToWater = -128.0f;
        float waterDepth = 0.0f;
        float waterSurfaceHeight = 0.0f;
        float moisture = 0.0f;
        float snowRetention = 0.0f;
        bool underwater = false;
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

        // CPU terrain-evolution controls. The generated ridge/noise field is
        // only the geological starting point; these settings shape glacial
        // troughs and derive a connected priority-flood/D8 drainage network.
        float glacialValleyStrength = 0.82f;
        float riverStartArea = 180000.0f; // upstream catchment area in m^2
        float riverBaseWidth = 3.5f;
        float riverWidthPerSqrtKm2 = 10.0f;
        float riverMaximumWidth = 34.0f;
        float riverBaseDepth = 0.75f;
        float riverDepthPerSqrtKm2 = 1.85f;
        float minimumLakeArea = 6500.0f;
        float minimumLakeDepth = 1.25f;

        // Glacial overdeepening anchors. They sculpt cirques/basins only;
        // priority-flood hydrology decides whether they hold water, their
        // spill elevation, shoreline and outlet.
        glm::vec2 lakeCenter = glm::vec2(0.0f, 0.0f);
        glm::vec2 lakeRadii = glm::vec2(460.0f, 310.0f);
        float lakeBasinDepth = 68.0f;
        float lakeRimHeight = 24.0f;

        // Separate meadow-side glacial overdeepening and camera landmark.
        glm::vec2 meadowLakeCenter = glm::vec2(-2200.0f, 1300.0f);
        glm::vec2 meadowLakeRadii = glm::vec2(320.0f, 220.0f);
        float meadowLakeBasinDepth = 28.0f;
        float meadowLakeRimHeight = 14.0f;
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
    // Bilinear CPU view of the immutable Terrain Data Map:
    // x=normalized height, y=slope, z=aspect, w=curvature.
    glm::vec4 sampleTerrainData(float worldX, float worldZ) const;
    // x=catchment-driven moisture, y=terrain-aware retained snow.
    glm::vec2 sampleEnvironmentData(float worldX, float worldZ) const;
    SurfaceSample sampleSurface(float worldX, float worldZ) const;
    bool isBelowWater(float worldX, float worldZ) const;
    glm::vec2 sampleLakeData(float worldX, float worldZ) const;
    float sampleWaterDepth(float worldX, float worldZ) const;
    float sampleSignedDistanceToWater(float worldX, float worldZ) const;
    float sampleWaterSurfaceHeight(float worldX, float worldZ) const;
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
    unsigned int getTerrainEnvironmentTexture() const { return terrainEnvironmentTexture; }
    unsigned int getLakeDataTexture() const { return lakeDataTexture; }
    glm::vec4 getLakeBoundsXZ() const { return lakeBoundsXZ; }
    float getLakeArea() const { return lakeArea; }
    float getMaximumWaterDepth() const { return maximumWaterDepth; }
    const std::vector<LakeRegion>& getLakeRegions() const { return lakeRegions; }
    bool wasLoadedFromCache() const { return cacheHit; }
    std::uint64_t getParameterHash() const { return parameterHash(); }

private:
    Settings settings;
    Material rockMaterial;
    Material grassMaterial;
    Material snowMaterial;
    std::unique_ptr<Mesh> mesh;
    std::vector<float> heightSamples;
    std::vector<float> terrainData; // transient derived-field workspace
    std::vector<std::uint16_t> terrainDataHalf;
    // RG: catchment-driven moisture and retained-snow potential.
    std::vector<std::uint16_t> terrainEnvironmentHalf;
    // RGB: depth, signed shore distance, local water-surface elevation.
    std::vector<std::uint16_t> lakeDataHalf;
    std::vector<LakeRegion> lakeRegions;
    unsigned int terrainDataTexture = 0;
    unsigned int terrainEnvironmentTexture = 0;
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
    void sculptGlacialLandforms();
    void generateHydrology();
    void computeDerivedFields();
    void computeEnvironmentFields();
    void buildMesh();
    void uploadTerrainTextures();
    void generateLakeData();
    std::uint64_t parameterHash() const;
    bool loadCache();
    void saveCache() const;

    // Generation-only fields, released after the cache payload is built.
    std::vector<float> flowAccumulation;
    std::vector<float> hydrologyWaterDepth;
    std::vector<float> hydrologyWaterSurface;
};
