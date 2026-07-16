#include "rendering/assets/mesh/WaterMesh.h"

#include <algorithm>
#include <vector>

namespace
{
std::unique_ptr<Mesh> buildWaterSurface(const TerrainMesh& terrain,
                                        unsigned int resolution)
{
    resolution = std::max(resolution, 3u);
    const float terrainSize = terrain.getSettings().size;
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;

    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, -1.0f);
    const std::vector<TerrainMesh::LakeRegion>& lakes = terrain.getLakeRegions();
    for (std::size_t lakeIndex = 0; lakeIndex < lakes.size(); ++lakeIndex)
    {
        const TerrainMesh::LakeRegion& lake = lakes[lakeIndex];
        const glm::vec4 bounds = lake.boundsXZ;
        const unsigned int patchResolution = lakeIndex == 0u
            ? resolution : std::max(96u, resolution * 2u / 3u);
        const unsigned int firstVertex = static_cast<unsigned int>(vertices.size());
        for (unsigned int z = 0; z < patchResolution; ++z)
        {
            const float v = static_cast<float>(z) /
                            static_cast<float>(patchResolution - 1u);
            const float worldZ = glm::mix(bounds.y, bounds.w, v);
            for (unsigned int x = 0; x < patchResolution; ++x)
            {
                const float u = static_cast<float>(x) /
                                static_cast<float>(patchResolution - 1u);
                const float worldX = glm::mix(bounds.x, bounds.z, u);
                const glm::vec2 lakeUV = glm::vec2(worldX, worldZ) /
                                         terrainSize + 0.5f;
                vertices.push_back({glm::vec3(worldX, lake.waterLevel, worldZ),
                                    normal, lakeUV, lakeUV, tangent, bitangent});
            }
        }
        for (unsigned int z = 0; z + 1u < patchResolution; ++z)
        {
            for (unsigned int x = 0; x + 1u < patchResolution; ++x)
            {
                const unsigned int a = firstVertex + z * patchResolution + x;
                const unsigned int b = a + 1u;
                const unsigned int c = a + patchResolution;
                const unsigned int d = c + 1u;
                indices.insert(indices.end(), {a, c, b, b, c, d});
            }
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
    : terrain(sourceTerrain), mesh(buildWaterSurface(terrain, 192u))
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
