#pragma once

#include <memory>

#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/modelload/Mesh.h"

class WaterMesh
{
public:
    explicit WaterMesh(const TerrainMesh& terrain);
    void updateStreaming(const glm::vec3& cameraPosition);
    void draw(Shader& shader) const;
    float getWaterLevel() const { return terrain.getWaterLevel(); }

private:
    const TerrainMesh& terrain;
    std::unique_ptr<Mesh> mesh;
};
