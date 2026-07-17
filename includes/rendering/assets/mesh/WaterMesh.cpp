#include "rendering/assets/mesh/WaterMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
struct ShoreVertex
{
    glm::vec2 position;
    float signedDistance;
};

void appendWaterVertex(std::vector<Mesh::Vertex>& vertices,
                       const ShoreVertex& source, float terrainSize)
{
    const glm::vec2 waterUV = source.position / terrainSize + 0.5f;
    vertices.push_back({glm::vec3(source.position.x, 0.0f, source.position.y),
                        glm::vec3(0.0f, 1.0f, 0.0f), waterUV, waterUV,
                        glm::vec3(1.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 0.0f, -1.0f)});
}

// Clip one positively-wound XZ triangle against the zero contour of the
// signed shore-distance field. This is marching-triangles with an interpolated
// crossing, used only by boundary cells; fully wet cells remain two quads.
void appendClippedTriangle(const std::array<ShoreVertex, 3>& triangle,
                           float terrainSize,
                           std::vector<Mesh::Vertex>& vertices,
                           std::vector<unsigned int>& indices)
{
    std::vector<ShoreVertex> polygon;
    polygon.reserve(4u);
    ShoreVertex previous = triangle.back();
    bool previousInside = previous.signedDistance >= 0.0f;
    for (const ShoreVertex& current : triangle)
    {
        const bool currentInside = current.signedDistance >= 0.0f;
        if (currentInside != previousInside)
        {
            const float denominator = previous.signedDistance -
                                      current.signedDistance;
            const float t = std::abs(denominator) > 1e-6f
                ? glm::clamp(previous.signedDistance / denominator,
                             0.0f, 1.0f)
                : 0.5f;
            polygon.push_back({glm::mix(previous.position, current.position, t),
                               0.0f});
        }
        if (currentInside)
            polygon.push_back(current);
        previous = current;
        previousInside = currentInside;
    }
    if (polygon.size() < 3u)
        return;

    const unsigned int first = static_cast<unsigned int>(vertices.size());
    for (const ShoreVertex& vertex : polygon)
        appendWaterVertex(vertices, vertex, terrainSize);
    for (unsigned int i = 1u; i + 1u < polygon.size(); ++i)
        indices.insert(indices.end(), {first, first + i, first + i + 1u});
}

std::unique_ptr<Mesh> buildWaterSurface(const TerrainMesh& terrain,
                                        unsigned int resolution)
{
    resolution = std::max(resolution, 3u);
    const float terrainSize = terrain.getSettings().size;
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;

    const float worldMinimum = -terrainSize * 0.5f;
    const float spacing = terrainSize / static_cast<float>(resolution - 1u);
    for (unsigned int z = 0; z + 1u < resolution; ++z)
    {
        const float worldZ = worldMinimum + z * spacing;
        for (unsigned int x = 0; x + 1u < resolution; ++x)
        {
            const float worldX = worldMinimum + x * spacing;
            const std::array<glm::vec2, 4> positions{{
                glm::vec2(worldX, worldZ),
                glm::vec2(worldX + spacing, worldZ),
                glm::vec2(worldX + spacing, worldZ + spacing),
                glm::vec2(worldX, worldZ + spacing)
            }};
            std::array<ShoreVertex, 4> corners;
            unsigned int wetCorners = 0u;
            for (unsigned int i = 0u; i < corners.size(); ++i)
            {
                corners[i] = {positions[i], terrain.sampleSignedDistanceToWater(
                    positions[i].x, positions[i].y)};
                wetCorners += corners[i].signedDistance >= 0.0f ? 1u : 0u;
            }
            const glm::vec2 centerPosition(worldX + spacing * 0.5f,
                                           worldZ + spacing * 0.5f);
            const ShoreVertex center{centerPosition,
                terrain.sampleSignedDistanceToWater(centerPosition.x,
                                                    centerPosition.y)};
            if (wetCorners == 0u && center.signedDistance < 0.0f)
                continue;

            if (wetCorners == 4u && center.signedDistance >= 0.0f)
            {
                const unsigned int first =
                    static_cast<unsigned int>(vertices.size());
                for (const ShoreVertex& corner : corners)
                    appendWaterVertex(vertices, corner, terrainSize);
                // Clockwise in XZ produces a +Y front face.
                indices.insert(indices.end(), {first, first + 3u, first + 1u,
                                               first + 1u, first + 3u,
                                               first + 2u});
                continue;
            }

            // A center fan resolves ambiguous marching-squares saddle cases
            // without joining two unrelated water lobes across a dry corner.
            for (unsigned int edge = 0u; edge < 4u; ++edge)
            {
                const unsigned int next = (edge + 1u) % 4u;
                appendClippedTriangle({center, corners[next], corners[edge]},
                                      terrainSize, vertices, indices);
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
    : terrain(sourceTerrain), mesh(buildWaterSurface(terrain, 1024u))
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
