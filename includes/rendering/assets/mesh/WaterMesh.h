#pragma once

#include <memory>

#include "rendering/modelload/Mesh.h"

class WaterMesh
{
public:
    static constexpr float RadiusX = 30.0f;
    static constexpr float RadiusZ = 25.2f;
    static constexpr float CenterZ = -18.0f;

    WaterMesh();
    void draw(Shader& shader) const;

private:
    std::unique_ptr<Mesh> mesh;
};
