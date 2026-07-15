#pragma once

#include <memory>
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
        unsigned int resolution = 321;
        float size = 300.0f;
        float mountainHeight = 28.0f;
        float baseHeight = -3.2f;
        unsigned int seed = 1337;
    };

    TerrainMesh();
    explicit TerrainMesh(const Settings& settings);
    ~TerrainMesh();

    TerrainMesh(const TerrainMesh&) = delete;
    TerrainMesh& operator=(const TerrainMesh&) = delete;

    void draw(Shader& shader) const;
    float sampleHeight(float worldX, float worldZ) const;
    glm::vec3 sampleNormal(float worldX, float worldZ) const;
    bool isBelowWater(float worldX, float worldZ) const;
    static constexpr float WaterLevel = -3.0f;
    const Settings& getSettings() const { return settings; }

private:
    Settings settings;
    Material terrainMaterial;
    Material grassMaterial;
    std::unique_ptr<Mesh> mesh;
    std::vector<float> heightSamples;
    unsigned int blendMaskTexture = 0;

    void generate();
};
