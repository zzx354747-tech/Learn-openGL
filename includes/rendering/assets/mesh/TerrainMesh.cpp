#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
float smoothStep(float a, float b, float x)
{
    const float t = glm::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

std::uint32_t hash2(int x, int y, std::uint32_t seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * 0x8da6b343u;
    h ^= static_cast<std::uint32_t>(y) * 0xd8163841u;
    h ^= seed * 0xcb1ab31fu;
    h ^= h >> 13;
    h *= 0x85ebca6bu;
    return h ^ (h >> 16);
}

float random01(int x, int y, std::uint32_t seed)
{
    return static_cast<float>(hash2(x, y, seed) & 0x00ffffffu) /
           static_cast<float>(0x00ffffffu);
}

float valueNoise(float x, float y, std::uint32_t seed)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);
    const float sx = tx * tx * (3.0f - 2.0f * tx);
    const float sy = ty * ty * (3.0f - 2.0f * ty);
    const float a = glm::mix(random01(x0,     y0,     seed), random01(x0 + 1, y0,     seed), sx);
    const float b = glm::mix(random01(x0,     y0 + 1, seed), random01(x0 + 1, y0 + 1, seed), sx);
    return glm::mix(a, b, sy) * 2.0f - 1.0f;
}

float fbm(float x, float y, std::uint32_t seed)
{
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    for (int octave = 0; octave < 5; ++octave)
    {
        sum += valueNoise(x * frequency, y * frequency, seed + octave * 101u) * amplitude;
        frequency *= 2.03f;
        amplitude *= 0.5f;
    }
    return sum;
}

}

TerrainMesh::TerrainMesh()
    : TerrainMesh(Settings{})
{
}

TerrainMesh::TerrainMesh(const Settings& nextSettings)
    : settings(nextSettings)
{
    settings.resolution = std::max(3u, settings.resolution);
    terrainMaterial = Material::loadFromDirectory("../textures/PBR/Rock060_2K-PNG");
    grassMaterial = Material::loadFromDirectory("../textures/PBR/Grass005_2K-PNG");
    // Snow010A follows the ambientCG channel naming convention rather than
    // the lowercase Poly Haven convention used by Material::loadFromDirectory.
    snowMaterial.albedoTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Color.png");
    snowMaterial.normalTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_NormalGL.png");
    snowMaterial.roughnessTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Roughness.png");
    snowMaterial.aoTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_AmbientOcclusion.png");
    snowMaterial.heightTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Displacement.png");
    generate();
}

TerrainMesh::~TerrainMesh()
{
    if (blendMaskTexture)
        glDeleteTextures(1, &blendMaskTexture);
}

void TerrainMesh::draw(Shader& shader) const
{
    if (!mesh)
        return;

    mesh->Draw(shader);
}

void TerrainMesh::updateStreaming(
    const glm::vec3& cameraPosition,
    const glm::mat4& viewProjection)
{
    streamingTile = glm::ivec2(0);
    streamingCameraPosition = cameraPosition;

    const glm::vec4 row0(
        viewProjection[0][0], viewProjection[1][0],
        viewProjection[2][0], viewProjection[3][0]);
    const glm::vec4 row1(
        viewProjection[0][1], viewProjection[1][1],
        viewProjection[2][1], viewProjection[3][1]);
    const glm::vec4 row2(
        viewProjection[0][2], viewProjection[1][2],
        viewProjection[2][2], viewProjection[3][2]);
    const glm::vec4 row3(
        viewProjection[0][3], viewProjection[1][3],
        viewProjection[2][3], viewProjection[3][3]);
    frustumPlanes = {
        row3 + row0, row3 - row0,
        row3 + row1, row3 - row1,
        row3 + row2, row3 - row2};
    for (glm::vec4& plane : frustumPlanes)
    {
        const float length = glm::length(glm::vec3(plane));
        if (length > 0.00001f)
            plane /= length;
    }
    streamingValid = true;
}

bool TerrainMesh::isTileVisible(int tileX, int tileZ) const
{
    if (tileX != 0 || tileZ != 0)
        return false;
    const glm::vec3 center(0.0f, settings.mountainHeight * 0.36f, -18.0f);
    const float halfSize = settings.size * 0.5f;
    const float boundingRadius = std::sqrt(
        halfSize * halfSize * 2.0f +
        settings.mountainHeight * settings.mountainHeight);
    for (const glm::vec4& plane : frustumPlanes)
    {
        if (glm::dot(glm::vec3(plane), center) + plane.w <
            -boundingRadius)
        {
            return false;
        }
    }
    return true;
}

float TerrainMesh::tileDistance(int tileX, int tileZ) const
{
    if (tileX != 0 || tileZ != 0)
        return settings.size * 2.0f;
    return 0.0f;
}

bool TerrainMesh::useFullTerrainDetail(int tileX, int tileZ) const
{
    return tileDistance(tileX, tileZ) <= 720.0f;
}

bool TerrainMesh::useFullVegetationDetail(int tileX, int tileZ) const
{
    return tileDistance(tileX, tileZ) <= 480.0f;
}

glm::mat4 TerrainMesh::getTileTransform(int tileX, int tileZ) const
{
    (void)tileX;
    (void)tileZ;
    return glm::mat4(1.0f);
}

float TerrainMesh::sampleHeight(float worldX, float worldZ) const
{
    const unsigned int n = settings.resolution;
    if (heightSamples.empty() || n < 2)
        return settings.baseHeight;

    const float spacing = settings.size / static_cast<float>(n - 1);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + 18.0f + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f ||
        gridX > static_cast<float>(n - 1) || gridZ > static_cast<float>(n - 1))
        return settings.baseHeight;

    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const float a = glm::mix(heightSamples[z0 * n + x0],
                             heightSamples[z0 * n + x0 + 1], tx);
    const float b = glm::mix(heightSamples[(z0 + 1) * n + x0],
                             heightSamples[(z0 + 1) * n + x0 + 1], tx);
    return glm::mix(a, b, tz);
}

glm::vec3 TerrainMesh::sampleNormal(float worldX, float worldZ) const
{
    const float step = settings.size / static_cast<float>(settings.resolution - 1);
    const float dx = sampleHeight(worldX + step, worldZ) - sampleHeight(worldX - step, worldZ);
    const float dz = sampleHeight(worldX, worldZ + step) - sampleHeight(worldX, worldZ - step);
    return glm::normalize(glm::vec3(-dx / (2.0f * step), 1.0f, -dz / (2.0f * step)));
}

bool TerrainMesh::isInsideCentralTerrain(float worldX, float worldZ) const
{
    const float halfSize = settings.size * 0.5f;
    return std::abs(worldX) <= halfSize &&
           std::abs(worldZ + 18.0f) <= halfSize;
}

glm::vec2 TerrainMesh::mapWorldToCentral(
    float worldX,
    float worldZ) const
{
    return glm::vec2(worldX, worldZ);
}

float TerrainMesh::sampleGrassWeight(
    float worldX,
    float worldZ,
    float height,
    float normalY) const
{
    (void)normalY;
    // Altitude alone owns the biome: solid grass below the transition and
    // solid rock above it. Only this narrow band blends the two colours.
    constexpr float GrassBlendHalfWidth = 70.0f;
    const float lowland = 1.0f - smoothStep(
        GrassLine - GrassBlendHalfWidth,
        GrassLine + GrassBlendHalfWidth,
        height);
    const float dry = isBelowWater(worldX, worldZ)
        ? 0.0f
        : smoothStep(WaterLevel + 0.8f, WaterLevel + 5.0f, height);
    return glm::clamp(lowland * dry, 0.0f, 1.0f);
}

float TerrainMesh::sampleSnowWeight(
    float worldX,
    float worldZ,
    float height,
    float normalY) const
{
    (void)worldX;
    (void)worldZ;
    (void)normalY;
    // The upper transition blends rock into snow; everything above it is
    // fully snow-covered, independent of slope or procedural noise.
    constexpr float SnowBlendHalfWidth = 90.0f;
    return smoothStep(
        SnowLine - SnowBlendHalfWidth,
        SnowLine + SnowBlendHalfWidth,
        height);
}

float TerrainMesh::sampleWorldHeight(float worldX, float worldZ) const
{
    return sampleHeight(worldX, worldZ);
}

glm::vec3 TerrainMesh::sampleWorldNormal(float worldX, float worldZ) const
{
    const float step =
        settings.size / static_cast<float>(settings.resolution - 1);
    const float dx = sampleWorldHeight(worldX + step, worldZ) -
                     sampleWorldHeight(worldX - step, worldZ);
    const float dz = sampleWorldHeight(worldX, worldZ + step) -
                     sampleWorldHeight(worldX, worldZ - step);
    return glm::normalize(glm::vec3(
        -dx / (2.0f * step), 1.0f, -dz / (2.0f * step)));
}

bool TerrainMesh::isBelowWater(float worldX, float worldZ) const
{
    return sampleWaterMask(worldX, worldZ) > 0.22f &&
           sampleHeight(worldX, worldZ) < WaterLevel + 0.8f;
}

float TerrainMesh::sampleWaterMask(float worldX, float worldZ)
{
    const float dx = worldX - FujiCenterX;
    const float dz = worldZ - FujiCenterZ;
    const float distance = std::sqrt(dx * dx + dz * dz);
    const float angle = std::atan2(dz, dx);
    const float ringCenter = LakeRadiusX +
        std::sin(angle * 3.0f + 0.65f) * 105.0f +
        std::sin(angle * 7.0f - 1.10f) * 48.0f;
    const float halfWidth = LakeRadiusZ *
        (0.78f + 0.18f * std::sin(angle * 5.0f - 0.4f));
    const float radial = 1.0f - smoothStep(
        halfWidth * 0.68f, halfWidth, std::abs(distance - ringCenter));
    const float fragmentSignal =
        std::sin(angle * 3.0f + 0.78f) +
        std::sin(angle * 7.0f - 1.16f) * 0.58f +
        std::sin(angle * 13.0f + 0.31f) * 0.24f;
    const float fragments = smoothStep(-0.16f, 0.30f, fragmentSignal);
    const float mountainRing = radial * fragments;

    // Two meandering channels cross the new lowland rather than reading as
    // straight, artificial trenches. Their varying widths also create small
    // marshy reaches where the channels meet the lakes.
    const float riverCenterZ = 1420.0f +
        std::sin(worldX * 0.00125f) * 285.0f +
        std::sin(worldX * 0.00305f + 1.10f) * 92.0f;
    const float riverWidth = 82.0f +
        (0.5f + 0.5f * std::sin(worldX * 0.0022f - 0.70f)) * 38.0f;
    const float eastWestRiver = 1.0f - smoothStep(
        riverWidth * 0.72f, riverWidth,
        std::abs(worldZ - riverCenterZ));

    const float tributaryCenterX = 2740.0f +
        std::sin(worldZ * 0.00115f + 0.45f) * 245.0f +
        std::sin(worldZ * 0.00335f - 0.80f) * 75.0f;
    const float tributaryWidth = 62.0f +
        (0.5f + 0.5f * std::sin(worldZ * 0.0027f + 0.20f)) * 31.0f;
    const float northSouthRiver = 1.0f - smoothStep(
        tributaryWidth * 0.70f, tributaryWidth,
        std::abs(worldX - tributaryCenterX));

    const auto lakeMask = [](float x, float z,
                             float centerX, float centerZ,
                             float radiusX, float radiusZ,
                             float phase)
    {
        const float ox = x - centerX;
        const float oz = z - centerZ;
        const float angle = std::atan2(oz / radiusZ, ox / radiusX);
        const float outline = 1.0f +
            std::sin(angle * 3.0f + phase) * 0.075f +
            std::sin(angle * 7.0f - phase * 0.6f) * 0.035f;
        const float normalized = std::sqrt(
            (ox * ox) / (radiusX * radiusX) +
            (oz * oz) / (radiusZ * radiusZ)) / outline;
        return 1.0f - smoothStep(0.82f, 1.0f, normalized);
    };
    const float westLake = lakeMask(
        worldX, worldZ, -3050.0f, 1490.0f, 620.0f, 410.0f, 0.7f);
    const float eastLake = lakeMask(
        worldX, worldZ, 2920.0f, -1660.0f, 720.0f, 455.0f, 1.8f);
    const float northLake = lakeMask(
        worldX, worldZ, 2730.0f, 2240.0f, 440.0f, 305.0f, -0.4f);

    return glm::clamp(std::max({
        mountainRing, eastWestRiver, northSouthRiver,
        westLake, eastLake, northLake}), 0.0f, 1.0f);
}

void TerrainMesh::generate()
{
    const unsigned int n = settings.resolution;
    const float spacing = settings.size / static_cast<float>(n - 1);

    struct HorizonHill
    {
        float x;
        float z;
        float radius;
        float stretch;
        float height;
        float rotation;
    };
    const HorizonHill horizonHills[] = {
        {-2250.0f, -1810.0f, 1120.0f, 1.75f, 82.0f,  0.35f},
        { 2260.0f, -1740.0f, 1250.0f, 1.62f, 96.0f, -0.48f},
        {-2310.0f,   760.0f, 1080.0f, 1.86f, 68.0f,  1.05f},
        { 2290.0f,   910.0f, 1300.0f, 1.72f, 88.0f,  0.62f},
        {-1470.0f,  2160.0f, 1380.0f, 1.64f, 72.0f, -0.22f},
        { 1430.0f,  2200.0f, 1200.0f, 1.88f, 64.0f,  0.28f},
    };

    heightSamples.assign(n * n, settings.baseHeight);

    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = -settings.size * 0.5f + static_cast<float>(x) * spacing;
            const float worldZ = -18.0f - settings.size * 0.5f +
                                 static_cast<float>(z) * spacing;
            const float domainWarpX = fbm(
                worldX * 0.00062f, worldZ * 0.00062f,
                settings.seed + 11u) * 125.0f;
            const float domainWarpZ = fbm(
                worldX * 0.00062f + 9.3f,
                worldZ * 0.00062f - 4.7f,
                settings.seed + 29u) * 105.0f;
            const float fujiX = worldX + domainWarpX - FujiCenterX;
            const float fujiZ =
                (worldZ + domainWarpZ - FujiCenterZ) / 1.045f;
            const float radius = std::sqrt(fujiX * fujiX + fujiZ * fujiZ);
            const float angle = std::atan2(fujiZ, fujiX);
            const float outlineVariation =
                1.0f + std::sin(angle * 3.0f + 0.4f) * 0.045f +
                std::sin(angle * 7.0f - 1.2f) * 0.025f +
                fbm(worldX * 0.0009f, worldZ * 0.0009f,
                    settings.seed + 41u) * 0.035f;
            const float r = radius / (FujiRadius * outlineVariation);

            // One continuous Fuji silhouette occupies the northern half of the
            // original 5.1 km map. Concave lower flanks transition into a
            // steeper upper cone, with radial erosion kept below silhouette scale.
            const float footprint = 1.0f - smoothStep(0.88f, 1.035f, r);
            const float cone = std::pow(
                glm::clamp(1.0f - r, 0.0f, 1.0f), 1.38f);
            const float warp = fbm(
                worldX * 0.0012f, worldZ * 0.0012f,
                settings.seed + 17u);
            const float detail = fbm(
                worldX * 0.0065f + warp,
                worldZ * 0.0065f - warp,
                settings.seed + 53u);
            const float radialRidges =
                std::sin(angle * 13.0f + detail * 2.6f) * 0.48f +
                std::sin(angle * 29.0f - r * 10.0f) * 0.22f;
            const float drainage = -std::pow(
                0.5f + 0.5f * std::sin(
                    angle * 19.0f + detail * 3.0f + r * 5.0f),
                7.0f);
            const float erosionMask = smoothStep(0.07f, 0.24f, r) *
                                      (1.0f - smoothStep(0.88f, 1.01f, r));
            const float asymmetricShoulder = std::exp(
                -std::pow((r - 0.58f) / 0.20f, 2.0f)) *
                (std::sin(angle * 3.0f - 0.6f) * 42.0f +
                 std::sin(angle * 5.0f + 1.0f) * 18.0f);
            const float erosion =
                (detail * 38.0f + radialRidges * 46.0f +
                 drainage * 72.0f + asymmetricShoulder) *
                                  erosionMask;

            // A 90 m-wide summit crater and raised rim remain visible even
            // after the snow layer blends across the upper cone.
            const float crater = std::exp(-std::pow(r / 0.026f, 2.0f)) * 86.0f;
            const float rim = std::exp(
                -std::pow((r - 0.043f) / 0.018f, 2.0f)) *
                (26.0f + std::sin(angle * 5.0f) * 8.0f);
            const float fujiHeight = std::max(
                0.0f,
                (cone * settings.mountainHeight + erosion - crater + rim) *
                    footprint);

            // The expanded outer ring is a broad plain, not a flat plate. Two
            // low-frequency layers provide long agricultural-scale rolls while
            // keeping the horizon far below Fuji's foothills.
            const float rollingGround =
                fbm(worldX * 0.00034f, worldZ * 0.00034f,
                    settings.seed + 901u) * 27.0f +
                fbm(worldX * 0.00165f, worldZ * 0.00165f,
                    settings.seed + 977u) * 7.0f;

            float horizonHeight = 0.0f;
            for (const HorizonHill& hill : horizonHills)
            {
                const float c = std::cos(hill.rotation);
                const float s = std::sin(hill.rotation);
                const float ox = worldX - hill.x;
                const float oz = worldZ - hill.z;
                const float hx = ox * c - oz * s;
                const float hz = (ox * s + oz * c) / hill.stretch;
                const float hr = std::sqrt(hx * hx + hz * hz) / hill.radius;
                const float hillBody = std::exp(-hr * hr * 2.1f);
                const float hillRoughness = 0.82f + 0.18f * fbm(
                    worldX * 0.0022f, worldZ * 0.0022f,
                    settings.seed + static_cast<unsigned int>(hill.height));
                horizonHeight += hill.height * hillBody * hillRoughness;
            }

            // The same analytic mask carves the broken mountain ring, the
            // lowland rivers and all lakes before WaterMesh samples them.
            const unsigned int index = z * n + x;
            const float dryHeight = settings.baseHeight + rollingGround +
                                    horizonHeight + fujiHeight;
            const float waterMask = sampleWaterMask(worldX, worldZ);
            const float basinBlend = smoothStep(0.015f, 0.22f, waterMask);
            const float bedVariation =
                std::sin(worldX * 0.0081f + worldZ * 0.0037f) * 0.55f +
                std::sin(worldZ * 0.0113f - worldX * 0.0029f) * 0.35f;
            const float basinDepth = 2.8f +
                smoothStep(0.22f, 0.92f, waterMask) * 7.2f + bedVariation;
            const float basinHeight = std::min(
                dryHeight, WaterLevel - std::max(basinDepth, 2.2f));
            heightSamples[index] = glm::mix(
                dryHeight, basinHeight, basinBlend);
        }
    }

    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(n * n);
    // R = grass, G = snow. Rock is the remaining implicit weight.
    std::vector<unsigned char> blendPixels(n * n * 3u, 0u);
    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const unsigned int i = z * n + x;
            const unsigned int xl = x > 0 ? x - 1 : x;
            const unsigned int xr = x + 1 < n ? x + 1 : x;
            const unsigned int zd = z > 0 ? z - 1 : z;
            const unsigned int zu = z + 1 < n ? z + 1 : z;
            const float dx = static_cast<float>(xr - xl) * spacing;
            const float dz = static_cast<float>(zu - zd) * spacing;
            const float dhdx = (heightSamples[z * n + xr] - heightSamples[z * n + xl]) / dx;
            const float dhdz = (heightSamples[zu * n + x] - heightSamples[zd * n + x]) / dz;

            const glm::vec3 normal = glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
            const glm::vec3 tangent = glm::normalize(glm::vec3(1.0f, dhdx, 0.0f));
            const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));
            const glm::vec3 position(-settings.size * 0.5f + static_cast<float>(x) * spacing,
                                     heightSamples[i],
                                     -18.0f - settings.size * 0.5f + static_cast<float>(z) * spacing);
            // Both central and streamed meshes derive material UVs from the
            // same world coordinates. Restarting UVs at the central square was
            // one of the visible seams even when geometry heights matched.
            const glm::vec2 uv(position.x / 58.0f, position.z / 58.0f);
            const glm::vec2 blendUv(static_cast<float>(x) / static_cast<float>(n - 1),
                                    static_cast<float>(z) / static_cast<float>(n - 1));
            vertices.push_back({position, normal, uv, blendUv, tangent, bitangent});

            const float grassWeight = sampleGrassWeight(
                position.x, position.z, heightSamples[i], normal.y);
            const float snowWeight = sampleSnowWeight(
                position.x, position.z, heightSamples[i], normal.y);
            blendPixels[i * 3u] = static_cast<unsigned char>(
                glm::clamp(grassWeight * (1.0f - snowWeight), 0.0f, 1.0f) * 255.0f);
            blendPixels[i * 3u + 1u] = static_cast<unsigned char>(
                glm::clamp(snowWeight, 0.0f, 1.0f) * 255.0f);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve((n - 1) * (n - 1) * 6);
    for (unsigned int z = 0; z + 1 < n; ++z)
    {
        for (unsigned int x = 0; x + 1 < n; ++x)
        {
            const unsigned int a = z * n + x;
            const unsigned int b = a + 1;
            const unsigned int c = a + n;
            const unsigned int d = c + 1;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    MaterialFlags material;
    material.baseColorFactor = glm::vec4(1.0f);
    material.roughnessFactor = terrainMaterial.roughnessValue;
    material.metallicFactor = 0.0f;
    std::vector<Texture> textures;
    if (terrainMaterial.albedoTex)
        textures.push_back({terrainMaterial.albedoTex->getID(), "texture_baseColor", "terrain://rock/albedo"});
    if (terrainMaterial.normalTex)
        textures.push_back({terrainMaterial.normalTex->getID(), "texture_normal", "terrain://rock/normal"});
    if (terrainMaterial.roughnessTex)
        textures.push_back({terrainMaterial.roughnessTex->getID(), "texture_roughness", "terrain://rock/roughness"});
    if (terrainMaterial.metallicTex)
        textures.push_back({terrainMaterial.metallicTex->getID(), "texture_metallic", "terrain://rock/metallic"});
    // Terrain spans kilometres; parallaxing the base-rock height map before
    // blending all three biomes caused the distant striping seen in the
    // previous build. Geometry and per-layer normal maps provide the relief.

    glGenTextures(1, &blendMaskTexture);
    glBindTexture(GL_TEXTURE_2D, blendMaskTexture);
    // 769 RGB texels occupy 2307 bytes per row, which is not divisible by
    // OpenGL's default four-byte unpack alignment. Without this override each
    // row starts one byte late and cyclically reinterprets grass, snow and rock
    // channels, producing the visible green/white/grey contour stripes.
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, static_cast<GLsizei>(n), static_cast<GLsizei>(n),
                 0, GL_RGB, GL_UNSIGNED_BYTE, blendPixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    textures.push_back({blendMaskTexture, "texture_terrainBlend", "terrain://blend-mask"});
    if (grassMaterial.albedoTex)
        textures.push_back({grassMaterial.albedoTex->getID(), "texture_terrainGrassAlbedo", "terrain://grass/albedo"});
    if (grassMaterial.normalTex)
        textures.push_back({grassMaterial.normalTex->getID(), "texture_terrainGrassNormal", "terrain://grass/normal"});
    if (grassMaterial.roughnessTex)
        textures.push_back({grassMaterial.roughnessTex->getID(), "texture_terrainGrassRoughness", "terrain://grass/roughness"});
    if (grassMaterial.metallicTex)
        textures.push_back({grassMaterial.metallicTex->getID(), "texture_terrainGrassMetallic", "terrain://grass/metallic"});
    if (snowMaterial.albedoTex)
        textures.push_back({snowMaterial.albedoTex->getID(), "texture_terrainSnowAlbedo", "terrain://snow/albedo"});
    if (snowMaterial.normalTex)
        textures.push_back({snowMaterial.normalTex->getID(), "texture_terrainSnowNormal", "terrain://snow/normal"});
    if (snowMaterial.roughnessTex)
        textures.push_back({snowMaterial.roughnessTex->getID(), "texture_terrainSnowRoughness", "terrain://snow/roughness"});
    mesh = std::make_unique<Mesh>(std::move(vertices), std::move(indices),
                                  std::move(textures), material, glm::mat4(1.0f));
    generateLodMesh();
}

void TerrainMesh::generateLodMesh()
{
    if (!mesh || settings.resolution < 9)
        return;

    constexpr unsigned int Stride = 8;
    const unsigned int sourceResolution = settings.resolution;
    const unsigned int lodResolution =
        (sourceResolution - 1) / Stride + 1;
    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(lodResolution * lodResolution);
    for (unsigned int z = 0; z < lodResolution; ++z)
    {
        const unsigned int sourceZ = std::min(
            z * Stride, sourceResolution - 1);
        for (unsigned int x = 0; x < lodResolution; ++x)
        {
            const unsigned int sourceX = std::min(
                x * Stride, sourceResolution - 1);
            vertices.push_back(
                mesh->vertices[sourceZ * sourceResolution + sourceX]);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve((lodResolution - 1) * (lodResolution - 1) * 6);
    for (unsigned int z = 0; z + 1 < lodResolution; ++z)
    {
        for (unsigned int x = 0; x + 1 < lodResolution; ++x)
        {
            const unsigned int a = z * lodResolution + x;
            const unsigned int b = a + 1;
            const unsigned int c = a + lodResolution;
            const unsigned int d = c + 1;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }
    std::vector<Texture> lodTextures;
    for (const Texture& texture : mesh->textures)
    {
        if (texture.type == "texture_baseColor" ||
            texture.type == "texture_diffuse" ||
            texture.type == "texture_terrainBlend" ||
            texture.type == "texture_terrainGrassAlbedo" ||
            texture.type == "texture_terrainSnowAlbedo")
        {
            lodTextures.push_back(texture);
        }
    }
    lodMesh = std::make_unique<Mesh>(
        std::move(vertices), std::move(indices),
        std::move(lodTextures), mesh->flags, glm::mat4(1.0f));
}

#if 0
// Retired during visual validation: the world now reuses the central mesh
// directly. Kept temporarily only as patch history until validation completes.
void TerrainMesh::generateStreamingMeshes()
{
    const unsigned int n = std::clamp(
        settings.streamingDetailResolution, 65u, 257u);
    const float radius = std::max(settings.streamingDetailRadius, 400.0f);
    const float spacing = radius * 2.0f / static_cast<float>(n - 1);
    std::vector<float> detailHeights(n * n);
    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = streamingCenter.x - radius + x * spacing;
            const float worldZ = streamingCenter.y - radius + z * spacing;
            detailHeights[z * n + x] = sampleWorldHeight(worldX, worldZ);
        }
    }

    std::vector<Mesh::Vertex> detailVertices;
    detailVertices.reserve(n * n);
    std::vector<unsigned char> detailBlendPixels(n * n);
    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const unsigned int i = z * n + x;
            const unsigned int xl = x > 0 ? x - 1 : x;
            const unsigned int xr = x + 1 < n ? x + 1 : x;
            const unsigned int zd = z > 0 ? z - 1 : z;
            const unsigned int zu = z + 1 < n ? z + 1 : z;
            const float dx = std::max(static_cast<float>(xr - xl) * spacing, 0.001f);
            const float dz = std::max(static_cast<float>(zu - zd) * spacing, 0.001f);
            const float dhdx =
                (detailHeights[z * n + xr] - detailHeights[z * n + xl]) / dx;
            const float dhdz =
                (detailHeights[zu * n + x] - detailHeights[zd * n + x]) / dz;
            const glm::vec3 normal = glm::normalize(
                glm::vec3(-dhdx, 1.0f, -dhdz));
            const glm::vec3 tangent = glm::normalize(
                glm::vec3(1.0f, dhdx, 0.0f));
            const glm::vec3 bitangent = glm::normalize(
                glm::cross(normal, tangent));
            const float worldX = streamingCenter.x - radius + x * spacing;
            const float worldZ = streamingCenter.y - radius + z * spacing;
            const glm::vec2 uv(worldX / 4.0f, worldZ / 4.0f);
            const glm::vec2 blendUv(
                static_cast<float>(x) / static_cast<float>(n - 1),
                static_cast<float>(z) / static_cast<float>(n - 1));
            detailVertices.push_back({
                glm::vec3(worldX, detailHeights[i], worldZ),
                normal, uv, blendUv, tangent, bitangent});

            const float grassWeight = sampleGrassWeight(
                worldX, worldZ, detailHeights[i], normal.y);
            detailBlendPixels[i] = static_cast<unsigned char>(
                glm::clamp(grassWeight, 0.0f, 1.0f) * 255.0f);
        }
    }

    std::vector<unsigned int> detailIndices;
    detailIndices.reserve((n - 1) * (n - 1) * 6);
    for (unsigned int z = 0; z + 1 < n; ++z)
    {
        for (unsigned int x = 0; x + 1 < n; ++x)
        {
            const float centerX =
                streamingCenter.x - radius + (x + 0.5f) * spacing;
            const float centerZ =
                streamingCenter.y - radius + (z + 0.5f) * spacing;
            if (glm::distance(
                    glm::vec2(centerX, centerZ), streamingCenter) > radius ||
                isInsideCentralTerrain(centerX, centerZ))
            {
                continue;
            }
            const unsigned int a = z * n + x;
            const unsigned int b = a + 1;
            const unsigned int c = a + n;
            const unsigned int d = c + 1;
            detailIndices.insert(
                detailIndices.end(), {a, c, b, b, c, d});
        }
    }

    if (streamingBlendMaskTexture)
        glDeleteTextures(1, &streamingBlendMaskTexture);
    glGenTextures(1, &streamingBlendMaskTexture);
    glBindTexture(GL_TEXTURE_2D, streamingBlendMaskTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_R8,
        static_cast<GLsizei>(n), static_cast<GLsizei>(n),
        0, GL_RED, GL_UNSIGNED_BYTE, detailBlendPixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    MaterialFlags detailMaterial;
    detailMaterial.baseColorFactor = glm::vec4(1.0f);
    detailMaterial.roughnessFactor = terrainMaterial.roughnessValue;
    detailMaterial.metallicFactor = 0.0f;
    std::vector<Texture> detailTextures;
    if (terrainMaterial.albedoTex)
        detailTextures.push_back({terrainMaterial.albedoTex->getID(), "texture_baseColor", "terrain://stream/rock/albedo"});
    if (terrainMaterial.normalTex)
        detailTextures.push_back({terrainMaterial.normalTex->getID(), "texture_normal", "terrain://stream/rock/normal"});
    if (terrainMaterial.roughnessTex)
        detailTextures.push_back({terrainMaterial.roughnessTex->getID(), "texture_roughness", "terrain://stream/rock/roughness"});
    if (terrainMaterial.metallicTex)
        detailTextures.push_back({terrainMaterial.metallicTex->getID(), "texture_metallic", "terrain://stream/rock/metallic"});
    if (terrainMaterial.heightTex)
        detailTextures.push_back({terrainMaterial.heightTex->getID(), "texture_height", "terrain://stream/rock/height"});
    detailTextures.push_back({streamingBlendMaskTexture, "texture_terrainBlend", "terrain://stream/blend"});
    if (grassMaterial.albedoTex)
        detailTextures.push_back({grassMaterial.albedoTex->getID(), "texture_terrainGrassAlbedo", "terrain://stream/grass/albedo"});
    if (grassMaterial.normalTex)
        detailTextures.push_back({grassMaterial.normalTex->getID(), "texture_terrainGrassNormal", "terrain://stream/grass/normal"});
    if (grassMaterial.roughnessTex)
        detailTextures.push_back({grassMaterial.roughnessTex->getID(), "texture_terrainGrassRoughness", "terrain://stream/grass/roughness"});
    if (grassMaterial.metallicTex)
        detailTextures.push_back({grassMaterial.metallicTex->getID(), "texture_terrainGrassMetallic", "terrain://stream/grass/metallic"});
    streamingDetailMesh = std::make_unique<Mesh>(
        std::move(detailVertices), std::move(detailIndices),
        std::move(detailTextures), detailMaterial, glm::mat4(1.0f));

    const unsigned int rings = std::clamp(settings.farRingCount, 16u, 128u);
    const unsigned int segments = std::clamp(
        settings.farSegmentCount, 32u, 256u);
    const float innerRadius = radius * 0.92f;
    const float outerRadius = std::max(settings.worldRadius, radius * 2.0f);
    std::vector<Mesh::Vertex> farVertices;
    farVertices.reserve((rings + 1) * segments);
    for (unsigned int ring = 0; ring <= rings; ++ring)
    {
        const float amount = static_cast<float>(ring) /
            static_cast<float>(rings);
        const float ringRadius = glm::mix(
            innerRadius, outerRadius, std::pow(amount, 1.65f));
        for (unsigned int segment = 0; segment < segments; ++segment)
        {
            const float angle = static_cast<float>(segment) /
                static_cast<float>(segments) * 6.28318530718f;
            const float worldX = streamingCenter.x + std::cos(angle) * ringRadius;
            const float worldZ = streamingCenter.y + std::sin(angle) * ringRadius;
            const float height = sampleWorldHeight(worldX, worldZ);
            const glm::vec3 normal = sampleWorldNormal(worldX, worldZ);
            const glm::vec3 tangent = glm::normalize(
                glm::vec3(1.0f, -normal.x / std::max(normal.y, 0.05f), 0.0f));
            const glm::vec3 bitangent = glm::normalize(
                glm::cross(normal, tangent));
            const glm::vec2 uv(worldX / 40.0f, worldZ / 40.0f);
            farVertices.push_back({
                glm::vec3(worldX, height, worldZ),
                normal, uv, uv, tangent, bitangent});
        }
    }

    std::vector<unsigned int> farIndices;
    farIndices.reserve(rings * segments * 6);
    for (unsigned int ring = 0; ring < rings; ++ring)
    {
        const unsigned int inner = ring * segments;
        const unsigned int outer = (ring + 1) * segments;
        for (unsigned int segment = 0; segment < segments; ++segment)
        {
            const unsigned int next = (segment + 1) % segments;
            farIndices.insert(farIndices.end(), {
                inner + segment, outer + segment, inner + next,
                inner + next, outer + segment, outer + next});
        }
    }

    MaterialFlags farMaterial;
    // Far terrain keeps the dominant lowland material so simplification
    // changes sampling cost, not the world's biome/color identity.
    farMaterial.baseColorFactor = glm::vec4(1.0f);
    farMaterial.roughnessFactor = 0.96f;
    std::vector<Texture> farTextures;
    if (grassMaterial.albedoTex)
        farTextures.push_back({grassMaterial.albedoTex->getID(), "texture_baseColor", "terrain://far/albedo"});
    if (grassMaterial.roughnessTex)
        farTextures.push_back({grassMaterial.roughnessTex->getID(), "texture_roughness", "terrain://far/roughness"});
    farClipmapMesh = std::make_unique<Mesh>(
        std::move(farVertices), std::move(farIndices),
        std::move(farTextures), farMaterial, glm::mat4(1.0f));
}
#endif
