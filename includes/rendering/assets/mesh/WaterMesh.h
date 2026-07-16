#pragma once

#include <memory>

#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/modelload/Mesh.h"

class WaterMesh
{
public:
    explicit WaterMesh(const TerrainMesh& terrain);
    ~WaterMesh() = default;
    void updateStreaming(const glm::vec3& cameraPosition);
    void draw(Shader& shader) const;
    float getWaterLevel() const { return terrain.getWaterLevel(); }
    float getTerrainSize() const { return terrain.getSettings().size; }
    float getTerrainBaseHeight() const { return terrain.getSettings().baseHeight; }
    float getTerrainMountainHeight() const { return terrain.getSettings().mountainHeight; }
    unsigned int getTerrainDataTexture() const { return terrain.getTerrainDataTexture(); }
    unsigned int getLakeDataTexture() const { return terrain.getLakeDataTexture(); }
    const std::vector<TerrainMesh::LakeRegion>& getLakeRegions() const
    {
        return terrain.getLakeRegions();
    }
private:
    const TerrainMesh& terrain;
    std::unique_ptr<Mesh> mesh;
};
