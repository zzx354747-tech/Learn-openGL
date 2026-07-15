#include "rendering/assets/mesh/VegetationMesh.h"
#include "rendering/assets/mesh/VegetationDistribution.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

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
}

void VegetationMesh::draw(Shader& shader) const
{
    if (grass) grass->Draw(shader);
    if (flowerStems) flowerStems->Draw(shader);
    for (const auto& petals : flowerPetals)
        if (petals) petals->Draw(shader);
    if (trunks) trunks->Draw(shader);
    if (crowns) crowns->Draw(shader);
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

    // Trees: rejection sampling keeps forests on lower, gentler ground.
    unsigned int treeCount = 0;
    for (unsigned int attempt = 0; attempt < 5000 && treeCount < 650; ++attempt)
    {
        const float x = glm::mix(-halfSize, halfSize, nextRandom(randomState));
        const float z = -18.0f + glm::mix(-halfSize, halfSize, nextRandom(randomState));
        const float y = terrain.sampleHeight(x, z);
        const glm::vec3 normal = terrain.sampleNormal(x, z);
        const float centralDistance = glm::length(glm::vec2(x, z + 18.0f));
        if (normal.y < 0.86f || y > 5.0f || centralDistance < 25.0f ||
            terrain.isBelowWater(x, z) ||
            nextRandom(randomState) < 0.22f)
            continue;

        const float scale = glm::mix(0.72f, 1.45f, nextRandom(randomState));
        const float trunkHeight = 2.1f * scale;
        addCylinder(trunkVertices, trunkIndices, glm::vec3(x, y, z), 0.16f * scale, trunkHeight);
        const glm::vec3 crownBase(x, y + trunkHeight * 0.55f, z);
        addCone(crownVertices, crownIndices, crownBase, 1.25f * scale, 2.55f * scale);
        addCone(crownVertices, crownIndices, crownBase + glm::vec3(0.0f, 1.05f * scale, 0.0f),
                0.92f * scale, 2.15f * scale);
        ++treeCount;
    }

    std::uint32_t meadowRandomState = terrain.getSettings().seed ^ 0x6d2b79f5u;
    const std::vector<VegetationPatch> grassPatches = buildVegetationPatches(
        terrain.getSettings().size, terrain.getSettings().seed, VegetationPatchKind::Grass);

    // Expanded grass colonies use random polar scattering. Density is preserved,
    // but there are no rows or repeating grid intervals to reveal the generator.
    constexpr float grassSpacing = 0.72f;
    for (const VegetationPatch& patch : grassPatches)
    {
        const unsigned int targetCount = static_cast<unsigned int>(
            Pi * patch.radius * patch.radius / (grassSpacing * grassSpacing));
        for (unsigned int candidate = 0; candidate < targetCount; ++candidate)
        {
            const float angle = nextRandom(meadowRandomState) * 2.0f * Pi;
            const float irregularEdge = 0.90f +
                std::sin(angle * 7.0f + patch.center.x * 0.1f) * 0.10f;
            const float radius = patch.radius * irregularEdge *
                                 std::pow(nextRandom(meadowRandomState), 0.62f);
            const float x = patch.center.x + std::cos(angle) * radius;
            const float z = patch.center.y + std::sin(angle) * radius;
            if (flowerRibbonInfluence(x, z) > 0.04f)
                continue;
            const float y = terrain.sampleHeight(x, z);
            const glm::vec3 normal = terrain.sampleNormal(x, z);
            if (normal.y < 0.88f || y > 6.0f || terrain.isBelowWater(x, z))
                continue;

            const float rotation = nextRandom(meadowRandomState) * Pi;
            const float height = glm::mix(0.72f, 1.45f, nextRandom(meadowRandomState));
            const float width = glm::mix(0.10f, 0.18f, nextRandom(meadowRandomState));
            addGrassBlade(grassVertices, grassIndices, glm::vec3(x, y + 0.015f, z),
                          rotation, width, height);
            addGrassBlade(grassVertices, grassIndices, glm::vec3(x, y + 0.015f, z),
                          rotation + Pi * 0.5f, width * 0.9f, height * 0.92f);
            addGrassBlade(grassVertices, grassIndices, glm::vec3(x, y + 0.015f, z),
                          rotation + Pi * 0.25f, width * 0.76f, height * 0.82f);
        }
    }

    // Each flower ribbon is also sampled randomly in both its length and width,
    // retaining the curved macro shape without visible crosswise rows.
    constexpr float flowerSpacing = 0.82f;
    const float flowerExtent = halfSize * 0.90f;
    for (const FlowerRibbon& ribbon : FlowerRibbons)
    {
        const unsigned int targetCount = static_cast<unsigned int>(
            (flowerExtent * 2.0f) * (ribbon.halfWidth * 2.0f) /
            (flowerSpacing * flowerSpacing));
        for (unsigned int candidate = 0; candidate < targetCount; ++candidate)
        {
            const float xBase = glm::mix(-flowerExtent, flowerExtent,
                                         nextRandom(meadowRandomState));
            const float centerZ = flowerRibbonCenter(ribbon, xBase);
            const float localWidth = ribbon.halfWidth *
                (0.88f + std::sin(xBase * 0.055f + ribbon.phase) * 0.12f);
            const float x = xBase + (nextRandom(meadowRandomState) - 0.5f) * 0.45f;
            const float z = centerZ + glm::mix(-localWidth, localWidth,
                                                nextRandom(meadowRandomState));
            const float y = terrain.sampleHeight(x, z);
            if (terrain.sampleNormal(x, z).y < 0.89f || y > 5.8f ||
                terrain.isBelowWater(x, z))
                continue;

            const float stemHeight = glm::mix(0.38f, 0.82f, nextRandom(meadowRandomState));
            const float stemRotation = nextRandom(meadowRandomState) * Pi;
            addGrassBlade(flowerStemVertices, flowerStemIndices,
                          glm::vec3(x, y + 0.02f, z), stemRotation, 0.018f, stemHeight);
            addGrassBlade(flowerStemVertices, flowerStemIndices,
                          glm::vec3(x, y + 0.02f, z), stemRotation + Pi * 0.5f,
                          0.014f, stemHeight * 0.98f);

            unsigned int colorIndex = ribbon.dominantColor;
            const float colorVariation = nextRandom(meadowRandomState);
            if (colorVariation > 0.95f)
                colorIndex = (colorIndex + 1u) % flowerPetals.size();
            else if (colorVariation > 0.90f)
                colorIndex = (colorIndex + flowerPetals.size() - 1u) % flowerPetals.size();
            addFlowerHead(flowerVertices[colorIndex], flowerIndices[colorIndex],
                          glm::vec3(x, y + stemHeight, z),
                          glm::mix(0.14f, 0.24f, nextRandom(meadowRandomState)),
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
    grassMaterial.baseColorFactor = gammaColor(glm::vec3(0.075f, 0.24f, 0.055f));
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
