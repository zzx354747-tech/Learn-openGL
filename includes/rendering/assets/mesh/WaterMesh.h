#pragma once

#include <memory>

#include "rendering/modelload/Mesh.h"
#include "rendering/assets/mesh/TerrainMesh.h"

class WaterMesh
{
public:
    static constexpr float RadiusX = TerrainMesh::LakeRadiusX;
    static constexpr float RadiusZ = TerrainMesh::LakeRadiusZ;
    static constexpr float CenterX = TerrainMesh::LakeCenterX;
    static constexpr float CenterZ = TerrainMesh::LakeCenterZ;

    explicit WaterMesh(const TerrainMesh& terrain);
    void updateStreaming(const glm::vec3& cameraPosition);
    void draw(Shader& shader) const;

private:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Mesh> lodMesh;
    const TerrainMesh& terrain;

    void generateLodMesh();
};
