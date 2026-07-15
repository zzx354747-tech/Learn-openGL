#include "rendering/assets/mesh/WaterMesh.h"
#include "rendering/assets/mesh/TerrainMesh.h"

#include <cmath>
#include <vector>

namespace
{
constexpr float Pi = 3.14159265358979323846f;
}

WaterMesh::WaterMesh()
{
    constexpr unsigned int rings = 48;
    constexpr unsigned int segments = 96;

    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, -1.0f);
    vertices.push_back({glm::vec3(0.0f, TerrainMesh::WaterLevel, CenterZ), normal,
                        glm::vec2(0.5f), glm::vec2(0.5f), tangent, bitangent});

    for (unsigned int ring = 1; ring <= rings; ++ring)
    {
        const float r = static_cast<float>(ring) / static_cast<float>(rings);
        for (unsigned int segment = 0; segment < segments; ++segment)
        {
            const float angle = static_cast<float>(segment) / static_cast<float>(segments) * 2.0f * Pi;
            const float x = std::cos(angle) * RadiusX * r;
            const float z = std::sin(angle) * RadiusZ * r;
            const glm::vec2 uv(x / (RadiusX * 2.0f) + 0.5f,
                               z / (RadiusZ * 2.0f) + 0.5f);
            vertices.push_back({glm::vec3(x, TerrainMesh::WaterLevel, CenterZ + z), normal,
                                uv, uv, tangent, bitangent});
        }
    }

    for (unsigned int segment = 0; segment < segments; ++segment)
        indices.insert(indices.end(), {0u, 1u + segment, 1u + (segment + 1u) % segments});

    for (unsigned int ring = 1; ring < rings; ++ring)
    {
        const unsigned int inner = 1u + (ring - 1u) * segments;
        const unsigned int outer = 1u + ring * segments;
        for (unsigned int segment = 0; segment < segments; ++segment)
        {
            const unsigned int next = (segment + 1u) % segments;
            indices.insert(indices.end(), {inner + segment, outer + segment, inner + next,
                                           inner + next, outer + segment, outer + next});
        }
    }

    MaterialFlags waterMaterial;
    waterMaterial.doubleSided = true;
    mesh = std::make_unique<Mesh>(std::move(vertices), std::move(indices),
                                  std::vector<Texture>{}, waterMaterial, glm::mat4(1.0f));
}

void WaterMesh::draw(Shader& shader) const
{
    if (mesh)
        mesh->Draw(shader);
}
