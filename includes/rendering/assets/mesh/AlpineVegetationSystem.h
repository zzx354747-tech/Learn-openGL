#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "rendering/assets/mesh/AlpineVegetationMeshFactory.h"
#include "rendering/assets/mesh/TerrainMesh.h"

class Camera;
class Shader;
struct SceneRenderConfig;

class AlpineVegetationSystem
{
public:
    struct Settings
    {
        std::uint32_t seed = 0x5a17b3c9u;
        std::uint32_t treeBudget = 20000;
        std::uint32_t shrubBudget = 30000;
        std::uint32_t grassBudget = 400000;
        std::uint32_t flowerBudget = 100000;
        std::uint32_t cushionBudget = 1200;
        float chunkSize = 256.0f;
    };

    explicit AlpineVegetationSystem(const TerrainMesh& terrain);
    AlpineVegetationSystem(const TerrainMesh& terrain, const Settings& settings);
    ~AlpineVegetationSystem();

    AlpineVegetationSystem(const AlpineVegetationSystem&) = delete;
    AlpineVegetationSystem& operator=(const AlpineVegetationSystem&) = delete;

    void beginFrame(const Camera& camera, int width, int height);
    void drawGeometry(Shader& shader, const Camera& camera,
                      const SceneRenderConfig& config) const;
    void drawDirectionalShadow(Shader& shader, const Camera& camera,
                               const SceneRenderConfig& config) const;
    void bindTerrainDensity(Shader& shader, int textureUnit = 20) const;

    // Replace this implementation with a moisture-field texture sampler after
    // the OpenGL 4.3 migration. Distribution call sites intentionally depend
    // only on this signature; runtime add/remove and GPU distribution are out
    // of scope for the current immutable OpenGL 3.3 system.
    float sampleDensityField(const glm::vec2& worldXZ) const;

    bool wasLoadedFromCache() const { return cacheHit_; }
    std::size_t instanceCount() const;
    unsigned int densityTexture() const { return densityTexture_; }

private:
    enum class Species : std::uint8_t
    {
        ConiferTall, ConiferBroad, ConiferSapling,
        ShrubRound, ShrubWindSwept,
        GrassA, GrassB, GrassC,
        FlowerStar, FlowerBell, FlowerSpike,
        CushionA, CushionB,
        Count
    };

    struct Instance
    {
        glm::vec4 posScale;
        glm::vec4 rotColor;
    };
    static_assert(sizeof(Instance) == 32, "Vegetation instance layout must remain 32 bytes");

    struct Chunk
    {
        std::uint32_t first = 0;
        std::uint32_t count = 0;
        glm::vec2 center = glm::vec2(0.0f);
    };

    struct GpuMesh
    {
        GLuint vao = 0;
        GLuint vertexBuffer = 0;
        GLuint indexBuffer = 0;
        GLsizei indexCount = 0;
    };

    struct Bucket
    {
        Species species = Species::GrassA;
        VegetationMeshLODSet cpuMeshes;
        // 0..2 = geometric LODs, 3 = shadow silhouette, 4 = far point sprite.
        std::array<GpuMesh, 5> meshes;
        std::vector<Instance> instances;
        std::vector<Chunk> chunks;
        GLuint instanceBuffer = 0;
    };

    const TerrainMesh& terrain_;
    Settings settings_;
    std::array<Bucket, static_cast<std::size_t>(Species::Count)> buckets_;
    std::vector<std::uint8_t> densityPixels_;
    GLuint densityTexture_ = 0;
    glm::mat4 currentViewProjection_ = glm::mat4(1.0f);
    glm::mat4 previousViewProjection_ = glm::mat4(1.0f);
    glm::vec3 frameCameraPosition_ = glm::vec3(0.0f);
    int frameViewportHeight_ = 1;
    float currentWindTime_ = 0.0f;
    float previousWindTime_ = 0.0f;
    bool frameInitialized_ = false;
    bool cacheHit_ = false;

    void buildMeshSets();
    void generateDistribution();
    void buildChunks(Bucket& bucket);
    void buildDensityTexture();
    void upload();
    void destroyGpuResources();
    bool loadCache();
    void saveCache() const;
    std::uint64_t settingsHash() const;
    void validateBiomeParity() const;
    void drawBuckets(Shader& shader, const Camera& camera,
                     const SceneRenderConfig& config, bool shadow) const;
};

static_assert(sizeof(AlpineVegetationSystem::Settings) >= 20,
              "Vegetation settings unexpectedly changed layout");
