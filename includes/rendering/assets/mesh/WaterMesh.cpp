#include "rendering/assets/mesh/WaterMesh.h"

#include <algorithm>
#include <array>
#include <vector>

namespace
{
struct ShorePoint
{
    glm::vec2 xz;
    float height;
};

ShorePoint shorelineIntersection(const ShorePoint& a, const ShorePoint& b,
                                 float waterLevel)
{
    const float denominator = b.height - a.height;
    const float amount = std::abs(denominator) > 1e-6f
        ? glm::clamp((waterLevel - a.height) / denominator, 0.0f, 1.0f)
        : 0.5f;
    return {glm::mix(a.xz, b.xz, amount), waterLevel};
}

void appendClippedTriangle(const std::array<ShorePoint, 3>& triangle,
                           float waterLevel,
                           std::vector<Mesh::Vertex>& vertices,
                           std::vector<unsigned int>& indices)
{
    std::vector<ShorePoint> polygon;
    polygon.reserve(5);
    ShorePoint previous = triangle.back();
    bool previousWet = previous.height < waterLevel;
    for (const ShorePoint& current : triangle)
    {
        const bool currentWet = current.height < waterLevel;
        if (currentWet != previousWet)
            polygon.push_back(shorelineIntersection(previous, current, waterLevel));
        if (currentWet)
            polygon.push_back(current);
        previous = current;
        previousWet = currentWet;
    }
    if (polygon.size() < 3)
        return;

    const unsigned int first = static_cast<unsigned int>(vertices.size());
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, -1.0f);
    for (const ShorePoint& point : polygon)
    {
        const glm::vec2 uv = point.xz * 0.0012f;
        vertices.push_back({glm::vec3(point.xz.x, waterLevel, point.xz.y),
                            normal, uv, uv, tangent, bitangent});
    }
    for (unsigned int i = 1; i + 1 < polygon.size(); ++i)
        indices.insert(indices.end(), {first, first + i, first + i + 1u});
}

std::unique_ptr<Mesh> buildWaterSurface(const TerrainMesh& terrain,
                                        unsigned int resolution)
{
    resolution = std::max(resolution, 3u);
    const float size = terrain.getSettings().size;
    const float spacing = size / static_cast<float>(resolution - 1u);
    const float minimum = -size * 0.5f;
    const float waterLevel = terrain.getWaterLevel();
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int z = 0; z + 1u < resolution; ++z)
    {
        for (unsigned int x = 0; x + 1u < resolution; ++x)
        {
            const float x0 = minimum + x * spacing;
            const float x1 = x0 + spacing;
            const float z0 = minimum + z * spacing;
            const float z1 = z0 + spacing;
            const ShorePoint a{{x0, z0}, terrain.sampleHeight(x0, z0)};
            const ShorePoint b{{x1, z0}, terrain.sampleHeight(x1, z0)};
            const ShorePoint c{{x0, z1}, terrain.sampleHeight(x0, z1)};
            const ShorePoint d{{x1, z1}, terrain.sampleHeight(x1, z1)};
            appendClippedTriangle({a, c, b}, waterLevel, vertices, indices);
            appendClippedTriangle({b, c, d}, waterLevel, vertices, indices);
        }
    }
    MaterialFlags material;
    material.doubleSided = true;
    return std::make_unique<Mesh>(std::move(vertices), std::move(indices),
                                  std::vector<Texture>{}, material,
                                  glm::mat4(1.0f));
}
}

WaterMesh::WaterMesh(const TerrainMesh& sourceTerrain)
    : terrain(sourceTerrain), mesh(buildWaterSurface(terrain, 513u))
{
}

void WaterMesh::draw(Shader& shader) const
{
    if (mesh)
        mesh->Draw(shader);
}

void WaterMesh::updateStreaming(const glm::vec3& cameraPosition)
{
    (void)cameraPosition;
}
