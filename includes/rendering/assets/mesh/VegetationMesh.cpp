#include "rendering/assets/mesh/VegetationMesh.h"
#include "rendering/assets/mesh/VegetationDistribution.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
constexpr float Pi = 3.14159265358979323846f;

float nextRandom(std::uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return static_cast<float>((state >> 8) & 0x00ffffffu) /
           static_cast<float>(0x00ffffffu);
}

glm::vec4 gammaColor(const glm::vec3& linearColor)
{
    return glm::vec4(glm::pow(linearColor, glm::vec3(1.0f / 2.2f)), 1.0f);
}

void addVertex(std::vector<Mesh::Vertex>& vertices,
               const glm::vec3& position,
               const glm::vec3& normal,
               const glm::vec2& uv)
{
    glm::vec3 tangent = std::abs(normal.y) > 0.95f
        ? glm::vec3(1.0f, 0.0f, 0.0f)
        : glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));
    vertices.push_back({position, normal, uv, uv, tangent, bitangent});
}

void addCylinder(std::vector<Mesh::Vertex>& vertices,
                 std::vector<unsigned int>& indices,
                 const glm::vec3& base,
                 float radius,
                 float height)
{
    constexpr unsigned int sides = 7;
    const unsigned int first = static_cast<unsigned int>(vertices.size());
    for (unsigned int i = 0; i <= sides; ++i)
    {
        const float angle = static_cast<float>(i) / static_cast<float>(sides) * 2.0f * Pi;
        const glm::vec3 normal(std::cos(angle), 0.0f, std::sin(angle));
        addVertex(vertices, base + normal * radius, normal,
                  glm::vec2(static_cast<float>(i) / sides, 0.0f));
        addVertex(vertices, base + normal * radius + glm::vec3(0.0f, height, 0.0f), normal,
                  glm::vec2(static_cast<float>(i) / sides, 1.0f));
    }
    for (unsigned int i = 0; i < sides; ++i)
    {
        const unsigned int a = first + i * 2;
        indices.insert(indices.end(), {a, a + 1, a + 2, a + 2, a + 1, a + 3});
    }
}

void addCone(std::vector<Mesh::Vertex>& vertices,
             std::vector<unsigned int>& indices,
             const glm::vec3& center,
             float radius,
             float height)
{
    constexpr unsigned int sides = 8;
    const unsigned int first = static_cast<unsigned int>(vertices.size());
    for (unsigned int i = 0; i < sides; ++i)
    {
        const float angle = static_cast<float>(i) / static_cast<float>(sides) * 2.0f * Pi;
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 normal = glm::normalize(glm::vec3(radial.x, radius / height, radial.z));
        addVertex(vertices, center + radial * radius, normal,
                  glm::vec2(static_cast<float>(i) / sides, 0.0f));
    }
    for (unsigned int i = 0; i < sides; ++i)
    {
        const float angle = (static_cast<float>(i) + 0.5f) / static_cast<float>(sides) * 2.0f * Pi;
        const glm::vec3 normal = glm::normalize(glm::vec3(std::cos(angle), radius / height, std::sin(angle)));
        addVertex(vertices, center + glm::vec3(0.0f, height, 0.0f), normal, glm::vec2(0.5f, 1.0f));
        indices.insert(indices.end(), {first + i, first + (i + 1) % sides,
                                       static_cast<unsigned int>(vertices.size() - 1)});
    }
}

void addGrassBlade(std::vector<Mesh::Vertex>& vertices,
                   std::vector<unsigned int>& indices,
                   const glm::vec3& base,
                   float angle,
                   float width,
                   float height)
{
    const glm::vec3 right(std::cos(angle) * width, 0.0f, std::sin(angle) * width);
    const glm::vec3 lean(-std::sin(angle) * height * 0.12f, height, std::cos(angle) * height * 0.12f);
    const glm::vec3 normal = glm::normalize(glm::cross(right, lean));
    const unsigned int first = static_cast<unsigned int>(vertices.size());
    addVertex(vertices, base - right, normal, glm::vec2(0.0f, 0.0f));
    addVertex(vertices, base + right, normal, glm::vec2(1.0f, 0.0f));
    addVertex(vertices, base + lean + right * 0.18f, normal, glm::vec2(0.6f, 1.0f));
    addVertex(vertices, base + lean - right * 0.18f, normal, glm::vec2(0.4f, 1.0f));
    indices.insert(indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
}

void addFlowerHead(std::vector<Mesh::Vertex>& vertices,
                   std::vector<unsigned int>& indices,
                   const glm::vec3& center,
                   float radius,
                   float rotation)
{
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    for (unsigned int petal = 0; petal < 5; ++petal)
    {
        const float angle = rotation + static_cast<float>(petal) / 5.0f * 2.0f * Pi;
        const glm::vec3 direction(std::cos(angle), 0.08f, std::sin(angle));
        const glm::vec3 side(-std::sin(angle), 0.0f, std::cos(angle));
        const unsigned int first = static_cast<unsigned int>(vertices.size());
        addVertex(vertices, center + side * radius * 0.22f, normal, glm::vec2(0.45f, 1.0f));
        addVertex(vertices, center - side * radius * 0.22f, normal, glm::vec2(0.55f, 1.0f));
        addVertex(vertices, center + direction * radius + side * radius * 0.38f,
                  normal, glm::vec2(1.0f, 1.0f));
        addVertex(vertices, center + direction * radius - side * radius * 0.38f,
                  normal, glm::vec2(0.0f, 1.0f));
        indices.insert(indices.end(), {first, first + 1, first + 2,
                                       first + 1, first + 3, first + 2});
    }
}
}

VegetationMesh::VegetationMesh(const TerrainMesh& terrain)
    : terrain(terrain)
{
    generate();
    generateLodPoints();
}

void VegetationMesh::draw(Shader& shader) const
{
    const glm::ivec2 centerTile = terrain.getStreamingTile();
    GLint previousFrontFace = GL_CCW;
    glGetIntegerv(GL_FRONT_FACE, &previousFrontFace);
    for (int tileZ = centerTile.y - TerrainMesh::StreamingTileRadius;
         tileZ <= centerTile.y + TerrainMesh::StreamingTileRadius; ++tileZ)
    {
        for (int tileX = centerTile.x - TerrainMesh::StreamingTileRadius;
             tileX <= centerTile.x + TerrainMesh::StreamingTileRadius; ++tileX)
        {
            if (!terrain.isTileVisible(tileX, tileZ))
                continue;
            const bool reversesWinding =
                ((tileX & 1) != 0) != ((tileZ & 1) != 0);
            glFrontFace(reversesWinding ? GL_CW : GL_CCW);
            const glm::mat4 transform =
                terrain.getTileTransform(tileX, tileZ);
            if (terrain.useFullVegetationDetail(tileX, tileZ))
            {
                if (grass) grass->Draw(shader, transform);
                if (flowerStems) flowerStems->Draw(shader, transform);
                for (const auto& petals : flowerPetals)
                    if (petals) petals->Draw(shader, transform);
                if (trunks) trunks->Draw(shader, transform);
                if (crowns) crowns->Draw(shader, transform);
            }
            else
            {
                if (lodGrass) lodGrass->Draw(shader, transform);
                for (const auto& flowers : lodFlowers)
                    if (flowers) flowers->Draw(shader, transform);
                if (lodTrees) lodTrees->Draw(shader, transform);
            }
        }
    }
    glFrontFace(previousFrontFace);
}

void VegetationMesh::generateLodPoints()
{
    const auto buildPoints = [](
        const Mesh* source,
        const MaterialFlags& flags,
        unsigned int vertexStride,
        unsigned int vertexOffset,
        float pointSize) -> std::unique_ptr<Mesh>
    {
        if (!source || source->vertices.empty())
            return nullptr;
        std::vector<Mesh::Vertex> vertices;
        for (unsigned int i = vertexOffset;
             i < static_cast<unsigned int>(source->vertices.size());
             i += std::max(vertexStride, 1u))
        {
            Mesh::Vertex point = source->vertices[i];
            point.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            point.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            point.Bitangent = glm::vec3(0.0f, 0.0f, -1.0f);
            vertices.push_back(point);
        }
        std::vector<unsigned int> indices(vertices.size());
        for (unsigned int i = 0;
             i < static_cast<unsigned int>(indices.size()); ++i)
        {
            indices[i] = i;
        }
        return std::make_unique<Mesh>(
            std::move(vertices), std::move(indices),
            std::vector<Texture>{}, flags, glm::mat4(1.0f),
            GL_POINTS, pointSize);
    };

    if (trunks && crowns)
        lodTrees = buildPoints(trunks.get(), crowns->flags, 16u, 1u, 2.0f);
    if (grass)
        lodGrass = buildPoints(grass.get(), grass->flags, 12u, 2u, 1.0f);
    for (unsigned int color = 0; color < lodFlowers.size(); ++color)
    {
        if (flowerPetals[color])
        {
            lodFlowers[color] = buildPoints(
                flowerPetals[color].get(),
                flowerPetals[color]->flags,
                20u, 0u, 1.0f);
        }
    }
}

void VegetationMesh::updateStreaming(const glm::vec3& cameraPosition)
{
    // Terrain owns the shared tile index. Vegetation uses the exact same
    // transforms and therefore needs no independent generator or transition.
    (void)cameraPosition;
}

void VegetationMesh::generate()
{
    std::vector<Mesh::Vertex> trunkVertices;
    std::vector<unsigned int> trunkIndices;
    std::vector<Mesh::Vertex> crownVertices;
    std::vector<unsigned int> crownIndices;
    std::vector<Mesh::Vertex> grassVertices;
    std::vector<unsigned int> grassIndices;
    std::vector<Mesh::Vertex> flowerStemVertices;
    std::vector<unsigned int> flowerStemIndices;
    std::array<std::vector<Mesh::Vertex>, 6> flowerVertices;
    std::array<std::vector<unsigned int>, 6> flowerIndices;

    std::uint32_t randomState = terrain.getSettings().seed ^ 0xa341316cu;
    const float halfSize = terrain.getSettings().size * 0.48f;

    // Trees form a broad subalpine belt and scattered lowland groves. Altitude,
    // slope and the shared water mask keep every trunk out of rivers and lakes.
    unsigned int treeCount = 0;
    for (unsigned int attempt = 0; attempt < 140000 && treeCount < 8200; ++attempt)
    {
        const float x = glm::mix(-halfSize, halfSize, nextRandom(randomState));
        const float z = -18.0f + glm::mix(-halfSize, halfSize, nextRandom(randomState));
        const float y = terrain.sampleHeight(x, z);
        const glm::vec3 normal = terrain.sampleNormal(x, z);
        const float highAltitudeFade = glm::clamp(
            (TerrainMesh::TreeLine - y) / 170.0f, 0.0f, 1.0f);
        const float forestCluster = glm::clamp(
            0.52f + std::sin(x * 0.0031f) * 0.18f +
            std::sin(z * 0.0047f + 0.8f) * 0.17f +
            std::sin((x + z) * 0.0019f - 1.3f) * 0.13f,
            0.08f, 1.0f);
        const float distanceFromFuji = glm::length(glm::vec2(
            x - TerrainMesh::FujiCenterX,
            z - TerrainMesh::FujiCenterZ));
        const float plainBoost = glm::smoothstep(
            TerrainMesh::FujiRadius * 1.08f,
            TerrainMesh::FujiRadius * 1.58f,
            distanceFromFuji);
        if (normal.y < 0.82f || y < TerrainMesh::WaterLevel + 3.0f ||
            y > TerrainMesh::TreeLine || terrain.isBelowWater(x, z) ||
            nextRandom(randomState) >
                (0.14f + highAltitudeFade * 0.52f + plainBoost * 0.18f) *
                    forestCluster)
            continue;

        const float scale = glm::mix(3.0f, 6.3f, nextRandom(randomState)) *
                            glm::mix(0.72f, 1.0f, highAltitudeFade);
        const float trunkHeight = 2.1f * scale;
        addCylinder(trunkVertices, trunkIndices, glm::vec3(x, y, z), 0.16f * scale, trunkHeight);
        const glm::vec3 crownBase(x, y + trunkHeight * 0.55f, z);
        addCone(crownVertices, crownIndices, crownBase, 1.25f * scale, 2.55f * scale);
        addCone(crownVertices, crownIndices, crownBase + glm::vec3(0.0f, 1.05f * scale, 0.0f),
                0.92f * scale, 2.15f * scale);
        ++treeCount;
    }

    std::uint32_t meadowRandomState =
        terrain.getSettings().seed ^ 0x6d2b79f5u;

    const std::vector<VegetationPatch> grassPatches = buildVegetationPatches(
        terrain.getSettings().size,
        terrain.getSettings().seed,
        VegetationPatchKind::Grass);

    // Restore the original region -> dense random fill distribution. Each
    // selected macro-grid region contains thousands of irregularly scattered
    // clumps; samples are no longer diluted uniformly over the entire map.
    constexpr float GrassPatchSpacing = 5.0f;
    for (const VegetationPatch& patch : grassPatches)
    {
        const unsigned int targetCount = static_cast<unsigned int>(
            Pi * patch.radius * patch.radius /
            (GrassPatchSpacing * GrassPatchSpacing));
        for (unsigned int candidate = 0; candidate < targetCount; ++candidate)
        {
            const float angle = nextRandom(meadowRandomState) * 2.0f * Pi;
            const float irregularEdge = 0.90f +
                std::sin(angle * 7.0f + patch.center.x * 0.002f) * 0.10f;
            const float radius = patch.radius * irregularEdge *
                std::pow(nextRandom(meadowRandomState), 0.62f);
            const float x = patch.center.x + std::cos(angle) * radius;
            const float z = patch.center.y + std::sin(angle) * radius;
            const float y = terrain.sampleHeight(x, z);
            const glm::vec3 normal = terrain.sampleNormal(x, z);
            if (normal.y < 0.86f ||
                y < TerrainMesh::WaterLevel + 1.5f ||
                y > TerrainMesh::GrassLine || terrain.isBelowWater(x, z))
            {
                continue;
            }

            const float rotation = nextRandom(meadowRandomState) * Pi;
            const float height = glm::mix(
                1.35f, 2.65f, nextRandom(meadowRandomState));
            const float width = glm::mix(
                0.88f, 1.58f, nextRandom(meadowRandomState));
            addGrassBlade(
                grassVertices, grassIndices,
                glm::vec3(x, y + 0.015f, z),
                rotation, width, height);
            addGrassBlade(
                grassVertices, grassIndices,
                glm::vec3(x, y + 0.015f, z),
                rotation + Pi * 0.5f, width * 0.90f, height * 0.92f);
            addGrassBlade(
                grassVertices, grassIndices,
                glm::vec3(x, y + 0.015f, z),
                rotation + Pi * 0.25f, width * 0.76f, height * 0.82f);
        }
    }

    const std::vector<VegetationPatch> flowerPatches = buildVegetationPatches(
        terrain.getSettings().size,
        terrain.getSettings().seed,
        VegetationPatchKind::Flowers);

    // Flowers use the same original two-stage rule: select a grid region, then
    // densely random-fill its interior. Every region keeps one dominant colour.
    constexpr float FlowerPatchSpacing = 4.0f;
    for (const VegetationPatch& patch : flowerPatches)
    {
        const unsigned int targetCount = static_cast<unsigned int>(
            Pi * patch.radius * patch.radius /
            (FlowerPatchSpacing * FlowerPatchSpacing));
        for (unsigned int candidate = 0; candidate < targetCount; ++candidate)
        {
            const float angle = nextRandom(meadowRandomState) * 2.0f * Pi;
            const float irregularEdge = 0.92f +
                std::sin(angle * 5.0f + patch.center.y * 0.002f) * 0.08f;
            const float radius = patch.radius * irregularEdge *
                std::sqrt(nextRandom(meadowRandomState));
            const float x = patch.center.x + std::cos(angle) * radius;
            const float z = patch.center.y + std::sin(angle) * radius;
            const float y = terrain.sampleHeight(x, z);
            if (terrain.sampleNormal(x, z).y < 0.88f ||
                y < TerrainMesh::WaterLevel + 1.2f || y > 430.0f ||
                terrain.isBelowWater(x, z))
            {
                continue;
            }

            const float stemHeight = glm::mix(
                0.50f, 1.05f, nextRandom(meadowRandomState));
            const float stemRotation = nextRandom(meadowRandomState) * Pi;
            addGrassBlade(
                flowerStemVertices, flowerStemIndices,
                glm::vec3(x, y + 0.02f, z),
                stemRotation, 0.018f, stemHeight);
            addGrassBlade(
                flowerStemVertices, flowerStemIndices,
                glm::vec3(x, y + 0.02f, z),
                stemRotation + Pi * 0.5f, 0.014f, stemHeight * 0.98f);

            unsigned int colorIndex = patch.dominantColor;
            const float colorVariation = nextRandom(meadowRandomState);
            const unsigned int colorCount = static_cast<unsigned int>(
                flowerPetals.size());
            if (colorVariation > 0.95f)
                colorIndex = (colorIndex + 1u) % colorCount;
            else if (colorVariation > 0.90f)
                colorIndex = (colorIndex + colorCount - 1u) % colorCount;
            addFlowerHead(
                flowerVertices[colorIndex], flowerIndices[colorIndex],
                glm::vec3(x, y + stemHeight, z),
                glm::mix(0.40f, 0.66f, nextRandom(meadowRandomState)),
                nextRandom(meadowRandomState) * Pi);
        }
    }

    MaterialFlags trunkMaterial;
    trunkMaterial.baseColorFactor = gammaColor(glm::vec3(0.16f, 0.075f, 0.025f));
    trunkMaterial.roughnessFactor = 0.9f;
    trunks = std::make_unique<Mesh>(std::move(trunkVertices), std::move(trunkIndices),
                                    std::vector<Texture>{}, trunkMaterial, glm::mat4(1.0f));

    MaterialFlags crownMaterial;
    crownMaterial.baseColorFactor = gammaColor(glm::vec3(0.035f, 0.16f, 0.045f));
    crownMaterial.roughnessFactor = 0.88f;
    crowns = std::make_unique<Mesh>(std::move(crownVertices), std::move(crownIndices),
                                    std::vector<Texture>{}, crownMaterial, glm::mat4(1.0f));

    MaterialFlags grassMaterial;
    grassMaterial.doubleSided = true;
    grassMaterial.baseColorFactor = gammaColor(glm::vec3(0.095f, 0.285f, 0.060f));
    grassMaterial.roughnessFactor = 0.96f;
    grassMaterial.windAffected = true;
    grassMaterial.windStrength = 0.42f;
    grassMaterial.windDirection = glm::normalize(glm::vec2(0.88f, 0.47f));
    grass = std::make_unique<Mesh>(std::move(grassVertices), std::move(grassIndices),
                                   std::vector<Texture>{}, grassMaterial, glm::mat4(1.0f));

    MaterialFlags stemMaterial;
    stemMaterial.doubleSided = true;
    stemMaterial.baseColorFactor = gammaColor(glm::vec3(0.055f, 0.20f, 0.045f));
    stemMaterial.roughnessFactor = 0.95f;
    stemMaterial.windAffected = true;
    stemMaterial.windStrength = 0.32f;
    stemMaterial.windDirection = grassMaterial.windDirection;
    flowerStems = std::make_unique<Mesh>(std::move(flowerStemVertices),
                                         std::move(flowerStemIndices),
                                         std::vector<Texture>{}, stemMaterial,
                                         glm::mat4(1.0f));

    const std::array<glm::vec3, 6> flowerPalette = {
        glm::vec3(0.95f, 0.12f, 0.20f), // red
        glm::vec3(1.00f, 0.48f, 0.08f), // orange
        glm::vec3(0.98f, 0.82f, 0.08f), // yellow
        glm::vec3(0.92f, 0.22f, 0.58f), // pink
        glm::vec3(0.42f, 0.20f, 0.92f), // violet
        glm::vec3(0.10f, 0.42f, 0.95f), // blue
    };
    for (unsigned int color = 0; color < flowerPetals.size(); ++color)
    {
        MaterialFlags petalMaterial;
        petalMaterial.doubleSided = true;
        petalMaterial.baseColorFactor = gammaColor(flowerPalette[color]);
        petalMaterial.roughnessFactor = 0.72f;
        petalMaterial.windAffected = true;
        petalMaterial.windStrength = 0.32f;
        petalMaterial.windDirection = grassMaterial.windDirection;
        flowerPetals[color] = std::make_unique<Mesh>(
            std::move(flowerVertices[color]), std::move(flowerIndices[color]),
            std::vector<Texture>{}, petalMaterial, glm::mat4(1.0f));
    }
}

#if 0
// Retired during visual validation. All tiles draw the original vegetation
// batches with the same transform as the original terrain mesh.
void VegetationMesh::generateStreaming()
{
    constexpr float Radius = 480.0f;
    std::vector<Mesh::Vertex> trunkVertices;
    std::vector<unsigned int> trunkIndices;
    std::vector<Mesh::Vertex> crownVertices;
    std::vector<unsigned int> crownIndices;
    std::vector<Mesh::Vertex> grassVertices;
    std::vector<unsigned int> grassIndices;
    std::vector<Mesh::Vertex> stemVertices;
    std::vector<unsigned int> stemIndices;
    std::array<std::vector<Mesh::Vertex>, 6> petalVertices;
    std::array<std::vector<unsigned int>, 6> petalIndices;

    std::uint32_t state = terrain.getSettings().seed ^ 0x91e10da5u;
    state ^= static_cast<std::uint32_t>(
        static_cast<int>(streamingCenter.x) * 73856093);
    state ^= static_cast<std::uint32_t>(
        static_cast<int>(streamingCenter.y) * 19349663);

    unsigned int treeCount = 0;
    for (unsigned int attempt = 0; attempt < 2200 && treeCount < 260; ++attempt)
    {
        const float angle = nextRandom(state) * 2.0f * Pi;
        const float distance = Radius * std::sqrt(nextRandom(state));
        const float x = streamingCenter.x + std::cos(angle) * distance;
        const float z = streamingCenter.y + std::sin(angle) * distance;
        if (terrain.isInsideCentralTerrain(x, z))
            continue;
        const float y = terrain.sampleWorldHeight(x, z);
        const glm::vec3 normal = terrain.sampleWorldNormal(x, z);
        if (normal.y < 0.84f || y > 65.0f ||
            y < TerrainMesh::WaterLevel + 0.6f || nextRandom(state) < 0.36f)
        {
            continue;
        }
        const float scale = glm::mix(0.82f, 1.60f, nextRandom(state));
        const float trunkHeight = 2.2f * scale;
        addCylinder(
            trunkVertices, trunkIndices, glm::vec3(x, y, z),
            0.17f * scale, trunkHeight);
        const glm::vec3 crownBase(x, y + trunkHeight * 0.55f, z);
        addCone(
            crownVertices, crownIndices, crownBase,
            1.30f * scale, 2.70f * scale);
        addCone(
            crownVertices, crownIndices,
            crownBase + glm::vec3(0.0f, 1.12f * scale, 0.0f),
            0.94f * scale, 2.20f * scale);
        ++treeCount;
    }

    for (unsigned int candidate = 0; candidate < 7200; ++candidate)
    {
        const float angle = nextRandom(state) * 2.0f * Pi;
        const float distance = Radius * std::sqrt(nextRandom(state));
        const float x = streamingCenter.x + std::cos(angle) * distance;
        const float z = streamingCenter.y + std::sin(angle) * distance;
        if (terrain.isInsideCentralTerrain(x, z))
            continue;
        const float y = terrain.sampleWorldHeight(x, z);
        const glm::vec3 normal = terrain.sampleWorldNormal(x, z);
        if (normal.y < 0.88f || y > 55.0f ||
            y < TerrainMesh::WaterLevel + 0.35f)
        {
            continue;
        }

        const float rotation = nextRandom(state) * Pi;
        if (nextRandom(state) < 0.16f)
        {
            const float stemHeight = glm::mix(
                0.42f, 0.88f, nextRandom(state));
            addGrassBlade(
                stemVertices, stemIndices,
                glm::vec3(x, y + 0.02f, z),
                rotation, 0.017f, stemHeight);
            const unsigned int color = static_cast<unsigned int>(
                nextRandom(state) * 5.999f);
            addFlowerHead(
                petalVertices[color], petalIndices[color],
                glm::vec3(x, y + stemHeight, z),
                glm::mix(0.14f, 0.25f, nextRandom(state)),
                rotation);
        }
        else
        {
            const float height = glm::mix(
                0.65f, 1.35f, nextRandom(state));
            const float width = glm::mix(
                0.09f, 0.17f, nextRandom(state));
            addGrassBlade(
                grassVertices, grassIndices,
                glm::vec3(x, y + 0.015f, z),
                rotation, width, height);
            addGrassBlade(
                grassVertices, grassIndices,
                glm::vec3(x, y + 0.015f, z),
                rotation + Pi * 0.5f, width * 0.86f, height * 0.91f);
        }
    }

    MaterialFlags trunkMaterial;
    trunkMaterial.baseColorFactor = gammaColor(glm::vec3(0.16f, 0.075f, 0.025f));
    trunkMaterial.roughnessFactor = 0.9f;
    streamingTrunks = std::make_unique<Mesh>(
        std::move(trunkVertices), std::move(trunkIndices),
        std::vector<Texture>{}, trunkMaterial, glm::mat4(1.0f));

    MaterialFlags crownMaterial;
    crownMaterial.baseColorFactor = gammaColor(glm::vec3(0.035f, 0.16f, 0.045f));
    crownMaterial.roughnessFactor = 0.88f;
    streamingCrowns = std::make_unique<Mesh>(
        std::move(crownVertices), std::move(crownIndices),
        std::vector<Texture>{}, crownMaterial, glm::mat4(1.0f));

    MaterialFlags grassMaterialFlags;
    grassMaterialFlags.doubleSided = true;
    grassMaterialFlags.baseColorFactor = gammaColor(glm::vec3(0.075f, 0.24f, 0.055f));
    grassMaterialFlags.roughnessFactor = 0.96f;
    grassMaterialFlags.windAffected = true;
    grassMaterialFlags.windStrength = 0.42f;
    grassMaterialFlags.windDirection = glm::normalize(glm::vec2(0.88f, 0.47f));
    streamingGrass = std::make_unique<Mesh>(
        std::move(grassVertices), std::move(grassIndices),
        std::vector<Texture>{}, grassMaterialFlags, glm::mat4(1.0f));

    MaterialFlags stemMaterial;
    stemMaterial.doubleSided = true;
    stemMaterial.baseColorFactor = gammaColor(glm::vec3(0.055f, 0.20f, 0.045f));
    stemMaterial.roughnessFactor = 0.95f;
    stemMaterial.windAffected = true;
    stemMaterial.windStrength = 0.32f;
    stemMaterial.windDirection = grassMaterialFlags.windDirection;
    streamingFlowerStems = std::make_unique<Mesh>(
        std::move(stemVertices), std::move(stemIndices),
        std::vector<Texture>{}, stemMaterial, glm::mat4(1.0f));

    const std::array<glm::vec3, 6> palette = {
        glm::vec3(0.95f, 0.12f, 0.20f),
        glm::vec3(1.00f, 0.48f, 0.08f),
        glm::vec3(0.98f, 0.82f, 0.08f),
        glm::vec3(0.92f, 0.22f, 0.58f),
        glm::vec3(0.42f, 0.20f, 0.92f),
        glm::vec3(0.10f, 0.42f, 0.95f),
    };
    for (unsigned int color = 0; color < palette.size(); ++color)
    {
        MaterialFlags material;
        material.doubleSided = true;
        material.baseColorFactor = gammaColor(palette[color]);
        material.roughnessFactor = 0.72f;
        material.windAffected = true;
        material.windStrength = 0.32f;
        material.windDirection = grassMaterialFlags.windDirection;
        streamingFlowerPetals[color] = std::make_unique<Mesh>(
            std::move(petalVertices[color]), std::move(petalIndices[color]),
            std::vector<Texture>{}, material, glm::mat4(1.0f));
    }
}
#endif
