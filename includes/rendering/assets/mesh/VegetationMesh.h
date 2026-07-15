#pragma once

#include <array>
#include <memory>

#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/modelload/Mesh.h"

// Deterministically scatters batched low-poly trees, clustered grass and flowers.
class VegetationMesh
{
public:
    explicit VegetationMesh(const TerrainMesh& terrain);

    VegetationMesh(const VegetationMesh&) = delete;
    VegetationMesh& operator=(const VegetationMesh&) = delete;

    void draw(Shader& shader) const;

private:
    const TerrainMesh& terrain;
    std::unique_ptr<Mesh> trunks;
    std::unique_ptr<Mesh> crowns;
    std::unique_ptr<Mesh> grass;
    std::unique_ptr<Mesh> flowerStems;
    std::array<std::unique_ptr<Mesh>, 6> flowerPetals;

    void generate();
};
