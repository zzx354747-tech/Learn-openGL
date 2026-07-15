#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

enum class VegetationPatchKind
{
    Grass,
    Flowers,
};

struct VegetationPatch
{
    glm::vec2 center; // world XZ
    float radius;
    unsigned int dominantColor;
};

struct FlowerRibbon
{
    float baseZ;
    float amplitude;
    float frequency;
    float phase;
    float halfWidth;
    unsigned int dominantColor;
};

inline constexpr std::array<FlowerRibbon, 5> FlowerRibbons = {{
    {-120.0f, 10.0f, 0.027f, 0.2f, 12.0f, 0u},
    { -72.0f, 14.0f, 0.023f, 1.4f, 11.0f, 3u},
    { -20.0f, 11.0f, 0.031f, 2.5f, 13.0f, 2u},
    {  34.0f, 16.0f, 0.021f, 0.8f, 12.0f, 4u},
    {  88.0f,  9.0f, 0.029f, 2.0f, 10.0f, 1u},
}};

inline float flowerRibbonCenter(const FlowerRibbon& ribbon, float worldX)
{
    return ribbon.baseZ + std::sin(worldX * ribbon.frequency + ribbon.phase) *
           ribbon.amplitude + std::sin(worldX * ribbon.frequency * 2.3f - ribbon.phase) * 2.2f;
}

inline float flowerRibbonInfluence(float worldX, float worldZ)
{
    float influence = 0.0f;
    for (const FlowerRibbon& ribbon : FlowerRibbons)
    {
        const float distance = std::abs(worldZ - flowerRibbonCenter(ribbon, worldX));
        const float t = glm::clamp((distance - ribbon.halfWidth * 0.82f) /
                                   (ribbon.halfWidth * 0.28f), 0.0f, 1.0f);
        const float smooth = t * t * (3.0f - 2.0f * t);
        influence = std::max(influence, 1.0f - smooth);
    }
    return influence;
}

inline std::uint32_t vegetationHash(unsigned int x, unsigned int z, std::uint32_t seed)
{
    std::uint32_t h = x * 0x8da6b343u ^ z * 0xd8163841u ^ seed * 0xcb1ab31fu;
    h ^= h >> 13;
    h *= 0x85ebca6bu;
    return h ^ (h >> 16);
}

inline float vegetationRandom(std::uint32_t value)
{
    return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x00ffffffu);
}

// Expanded grass colonies cover roughly 60% before the flower ribbons carve out
// exclusive space, leaving about 40-50% of usable lowland as visible grass piles.
inline std::vector<VegetationPatch> buildVegetationPatches(
    float terrainSize, std::uint32_t seed, VegetationPatchKind requestedKind)
{
    constexpr unsigned int gridSize = 7;
    const float halfExtent = terrainSize * 0.42f;
    const float cellSize = halfExtent * 2.0f / static_cast<float>(gridSize);
    std::vector<VegetationPatch> patches;

    for (unsigned int z = 0; z < gridSize; ++z)
    {
        for (unsigned int x = 0; x < gridSize; ++x)
        {
            const std::uint32_t h = vegetationHash(x, z, seed + 1709u);
            const float selector = vegetationRandom(h);
            if (requestedKind != VegetationPatchKind::Grass || selector >= 0.78f)
                continue;

            const float jitterX = (vegetationRandom(h * 1664525u + 1013904223u) - 0.5f) *
                                  cellSize * 0.30f;
            const float jitterZ = (vegetationRandom(h * 22695477u + 1u) - 0.5f) *
                                  cellSize * 0.30f;
            const float radiusRandom = vegetationRandom(h * 1103515245u + 12345u);
            const float radius = 16.0f + radiusRandom * 4.0f;
            patches.push_back({
                glm::vec2(-halfExtent + (static_cast<float>(x) + 0.5f) * cellSize + jitterX,
                          -18.0f - halfExtent + (static_cast<float>(z) + 0.5f) * cellSize + jitterZ),
                radius,
                (x * 3u + z * 5u + (h >> 24u)) % 6u
            });
        }
    }
    return patches;
}
