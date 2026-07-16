#include "rendering/assets/mesh/WaterMesh.h"
#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
std::unique_ptr<Mesh> buildWaterSurface(
    const TerrainMesh& terrain,
    unsigned int resolution)
{
    resolution = std::max(resolution, 3u);
    const float size = terrain.getSettings().size;
    const float spacing = size / static_cast<float>(resolution - 1u);
    const float minX = -size * 0.5f;
    const float minZ = -18.0f - size * 0.5f;
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(resolution * resolution);
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, -1.0f);

    for (unsigned int z = 0; z < resolution; ++z)
    {
        for (unsigned int x = 0; x < resolution; ++x)
        {
            const float worldX = minX + static_cast<float>(x) * spacing;
            const float worldZ = minZ + static_cast<float>(z) * spacing;
            const glm::vec2 uv(worldX * 0.0012f, worldZ * 0.0012f);
            vertices.push_back({
                glm::vec3(worldX, TerrainMesh::WaterLevel, worldZ),
                normal, uv, uv, tangent, bitangent});
        }
    }

    const auto isWet = [&terrain](float worldX, float worldZ)
    {
        return TerrainMesh::sampleWaterMask(worldX, worldZ) >= 0.22f &&
               terrain.sampleHeight(worldX, worldZ) <
                   TerrainMesh::WaterLevel + 0.8f;
    };
    for (unsigned int z = 0; z + 1u < resolution; ++z)
    {
        for (unsigned int x = 0; x + 1u < resolution; ++x)
        {
            const unsigned int a = z * resolution + x;
            const unsigned int b = a + 1u;
            const unsigned int c = a + resolution;
            const unsigned int d = c + 1u;
            const float x0 = minX + static_cast<float>(x) * spacing;
            const float z0 = minZ + static_cast<float>(z) * spacing;

            // Test each triangle at its centroid. This retains winding and
            // shared vertices while tracing narrow rivers more accurately than
            // accepting or rejecting a complete square.
            if (isWet(x0 + spacing / 3.0f, z0 + spacing / 3.0f))
                indices.insert(indices.end(), {a, c, b});
            if (isWet(x0 + spacing * 2.0f / 3.0f,
                      z0 + spacing * 2.0f / 3.0f))
                indices.insert(indices.end(), {b, c, d});
        }
    }

    MaterialFlags material;
    material.doubleSided = true;
    return std::make_unique<Mesh>(
        std::move(vertices), std::move(indices),
        std::vector<Texture>{}, material, glm::mat4(1.0f));
}
}

WaterMesh::WaterMesh(const TerrainMesh& sourceTerrain)
    : terrain(sourceTerrain)
{
    mesh = buildWaterSurface(terrain, 513u);
    generateLodMesh();
}

void WaterMesh::generateLodMesh()
{
    lodMesh = buildWaterSurface(terrain, 257u);
}

void WaterMesh::draw(Shader& shader) const
{
    if (!mesh)
        return;
    const glm::ivec2 centerTile = terrain.getStreamingTile();
    for (int tileZ = centerTile.y - TerrainMesh::StreamingTileRadius;
         tileZ <= centerTile.y + TerrainMesh::StreamingTileRadius; ++tileZ)
    {
        for (int tileX = centerTile.x - TerrainMesh::StreamingTileRadius;
             tileX <= centerTile.x + TerrainMesh::StreamingTileRadius; ++tileX)
        {
            if (!terrain.isTileVisible(tileX, tileZ))
                continue;
            Mesh* selectedMesh = terrain.useFullTerrainDetail(tileX, tileZ)
                ? mesh.get()
                : lodMesh.get();
            if (selectedMesh)
                selectedMesh->Draw(
                    shader, terrain.getTileTransform(tileX, tileZ));
        }
    }
}

void WaterMesh::updateStreaming(const glm::vec3& cameraPosition)
{
    // Water is generated once over the same expanded bounds as the terrain.
    (void)cameraPosition;
}

#if 0
// Retired during visual validation. Every tile now draws the exact original
// lake mesh instead of extracting an independent shoreline.
void WaterMesh::generateStreamingMesh()
{
    // Shorelines are extracted from the same continuous height function as the
    // terrain. The old 81x81 center-test emitted entire 27.5 m quads as water,
    // including quads crossing the central terrain boundary.
    constexpr unsigned int Resolution = 257;
    constexpr float Radius = 1100.0f;
    const float spacing = Radius * 2.0f /
        static_cast<float>(Resolution - 1);
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(Resolution * Resolution);
    indices.reserve(Resolution * Resolution * 3);
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    const glm::vec3 bitangent(0.0f, 0.0f, -1.0f);

    struct ShoreSample
    {
        glm::vec2 position;
        float height;
    };
    const auto intersection = [](const ShoreSample& a, const ShoreSample& b)
    {
        const float denominator = b.height - a.height;
        const float amount = std::abs(denominator) > 0.00001f
            ? glm::clamp(
                (TerrainMesh::WaterLevel - a.height) / denominator,
                0.0f, 1.0f)
            : 0.5f;
        return ShoreSample{
            glm::mix(a.position, b.position, amount),
            TerrainMesh::WaterLevel};
    };

    for (unsigned int z = 0; z + 1 < Resolution; ++z)
    {
        for (unsigned int x = 0; x + 1 < Resolution; ++x)
        {
            const float x0 = streamingCenter.x - Radius + x * spacing;
            const float x1 = x0 + spacing;
            const float z0 = streamingCenter.y - Radius + z * spacing;
            const float z1 = z0 + spacing;
            const float centerX = (x0 + x1) * 0.5f;
            const float centerZ = (z0 + z1) * 0.5f;
            if (glm::distance(
                    glm::vec2(centerX, centerZ), streamingCenter) > Radius ||
                terrain.isInsideCentralTerrain(x0, z0) ||
                terrain.isInsideCentralTerrain(x1, z0) ||
                terrain.isInsideCentralTerrain(x1, z1) ||
                terrain.isInsideCentralTerrain(x0, z1))
            {
                continue;
            }

            std::vector<ShoreSample> input = {
                {{x0, z0}, terrain.sampleWorldHeight(x0, z0)},
                {{x1, z0}, terrain.sampleWorldHeight(x1, z0)},
                {{x1, z1}, terrain.sampleWorldHeight(x1, z1)},
                {{x0, z1}, terrain.sampleWorldHeight(x0, z1)}};
            std::vector<ShoreSample> polygon;
            polygon.reserve(6);
            ShoreSample previous = input.back();
            bool previousInside = previous.height < TerrainMesh::WaterLevel;
            for (const ShoreSample& current : input)
            {
                const bool currentInside =
                    current.height < TerrainMesh::WaterLevel;
                if (currentInside != previousInside)
                    polygon.push_back(intersection(previous, current));
                if (currentInside)
                    polygon.push_back(current);
                previous = current;
                previousInside = currentInside;
            }
            if (polygon.size() < 3)
                continue;

            const unsigned int first =
                static_cast<unsigned int>(vertices.size());
            for (const ShoreSample& point : polygon)
            {
                const glm::vec2 uv = point.position * 0.002f;
                vertices.push_back({
                    glm::vec3(
                        point.position.x,
                        TerrainMesh::WaterLevel,
                        point.position.y),
                    normal, uv, uv, tangent, bitangent});
            }
            for (unsigned int i = 1;
                 i + 1 < static_cast<unsigned int>(polygon.size()); ++i)
            {
                indices.insert(indices.end(), {
                    first, first + i, first + i + 1});
            }
        }
    }

    MaterialFlags material;
    material.doubleSided = true;
    streamingMesh = std::make_unique<Mesh>(
        std::move(vertices), std::move(indices),
        std::vector<Texture>{}, material, glm::mat4(1.0f));
}
#endif
