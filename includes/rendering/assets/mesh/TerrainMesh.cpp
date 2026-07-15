#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

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
    // Reuse the exact Rock060 PBR set shown on the material spheres.
    terrainMaterial = Material::loadFromDirectory("../textures/PBR/Rock060_2K-PNG");
    grassMaterial = Material::loadFromDirectory("../textures/PBR/Grass005_2K-PNG");
    generate();
}

TerrainMesh::~TerrainMesh()
{
    if (blendMaskTexture)
        glDeleteTextures(1, &blendMaskTexture);
}

void TerrainMesh::draw(Shader& shader) const
{
    if (mesh)
        mesh->Draw(shader);
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

bool TerrainMesh::isBelowWater(float worldX, float worldZ) const
{
    const float dx = worldX;
    const float dz = (worldZ + 18.0f) / 0.84f;
    return std::sqrt(dx * dx + dz * dz) < 31.0f &&
           sampleHeight(worldX, worldZ) < WaterLevel + 0.35f;
}

void TerrainMesh::generate()
{
    const unsigned int n = settings.resolution;
    const float spacing = settings.size / static_cast<float>(n - 1);
    const float mountainRadius = 28.0f;

    heightSamples.assign(n * n, settings.baseHeight);

    struct Mountain
    {
        float x;
        float z;
        float radius;
        float height;
        float stretch;
        float rotation;
        unsigned int seedOffset;
    };

    // Secondary ranges sit well beyond the Fuji-like central peak and vary in
    // footprint, orientation and elevation so the horizon is not a flat ring.
    const Mountain surroundingMountains[] = {
        {-72.0f, -62.0f, 36.0f, 28.0f, 1.45f,  0.35f, 301u},
        { 63.0f, -78.0f, 44.0f, 34.0f, 1.20f, -0.55f, 401u},
        {-108.0f, 38.0f, 35.0f, 24.0f, 1.65f,  1.10f, 501u},
        { 102.0f, 42.0f, 40.0f, 27.0f, 1.35f,  0.70f, 601u},
        {-35.0f, 102.0f, 31.0f, 21.0f, 1.75f, -0.20f, 701u},
        { 34.0f, 112.0f, 39.0f, 25.0f, 1.25f,  0.25f, 801u},
    };

    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = -settings.size * 0.5f + static_cast<float>(x) * spacing;
            const float worldZ = -settings.size * 0.5f + static_cast<float>(z) * spacing;
            // Keep the Fuji-like peak off-center so the center can open into a lake basin.
            const float fujiX = worldX - 42.0f;
            const float fujiZ = worldZ + 55.0f;
            const float radius = std::sqrt(fujiX * fujiX + fujiZ * fujiZ);
            const float r = radius / mountainRadius;
            const float angle = std::atan2(fujiZ, fujiX);

            // Broad foot + nearly conical upper mountain. Domain-warped fBm and
            // angular ridges create erosion lines without losing Fuji's silhouette.
            const float footprint = 1.0f - smoothStep(0.82f, 1.12f, r);
            const float cone = std::pow(glm::clamp(1.0f - r, 0.0f, 1.0f), 1.18f);
            const float warp = fbm(worldX * 0.055f, worldZ * 0.055f, settings.seed + 17u);
            const float detail = fbm(worldX * 0.18f + warp, worldZ * 0.18f - warp,
                                     settings.seed + 53u);
            const float radialRidges =
                std::sin(angle * 13.0f + detail * 2.2f) * 0.55f +
                std::sin(angle * 29.0f - r * 7.0f) * 0.22f;
            const float erosion = (detail * 0.55f + radialRidges * 0.45f) *
                                  smoothStep(0.08f, 0.30f, r) *
                                  (1.0f - smoothStep(0.82f, 1.05f, r));

            // A shallow summit crater and a subtle rim keep the peak geological.
            const float crater = std::exp(-(r * r) / 0.0015f) * 0.085f;
            const float rim = std::exp(-std::pow((r - 0.055f) / 0.025f, 2.0f)) * 0.025f;
            float h01 = cone + erosion * 0.075f * footprint - crater + rim;
            h01 = std::max(0.0f, h01 * footprint);

            float surroundingHeight = 0.0f;
            for (const Mountain& mountain : surroundingMountains)
            {
                const float cosR = std::cos(mountain.rotation);
                const float sinR = std::sin(mountain.rotation);
                const float offsetX = worldX - mountain.x;
                const float offsetZ = worldZ - mountain.z;
                const float rotatedX = offsetX * cosR - offsetZ * sinR;
                const float rotatedZ = offsetX * sinR + offsetZ * cosR;
                const float mr = std::sqrt(rotatedX * rotatedX +
                                           (rotatedZ / mountain.stretch) *
                                           (rotatedZ / mountain.stretch)) / mountain.radius;
                const float mountainMask = 1.0f - smoothStep(0.72f, 1.05f, mr);
                const float mountainBody = std::pow(glm::clamp(1.0f - mr, 0.0f, 1.0f), 1.35f);
                const float mountainAngle = std::atan2(rotatedZ, rotatedX);
                const float mountainNoise = fbm(worldX * 0.045f, worldZ * 0.045f,
                                                 settings.seed + mountain.seedOffset);
                const float mountainRidges =
                    std::sin(mountainAngle * 9.0f + mountainNoise * 3.0f) * 0.055f +
                    std::sin(mountainAngle * 19.0f - mr * 5.0f) * 0.025f;
                surroundingHeight += mountain.height *
                    std::max(0.0f, mountainBody + mountainRidges * mountainMask);
            }

            // Two noise scales form rolling ground, shallow basins and rough
            // foothills between the explicit peaks.
            const float rollingGround =
                fbm(worldX * 0.017f, worldZ * 0.017f, settings.seed + 901u) * 2.4f +
                fbm(worldX * 0.060f, worldZ * 0.060f, settings.seed + 977u) * 0.75f;

            // Elliptical central basin: a deep smooth bowl surrounded by a raised,
            // noisy rim. The water plane intersects this shape into a shoreline.
            const float basinRadius = std::sqrt(worldX * worldX +
                (worldZ / 0.84f) * (worldZ / 0.84f));
            const float basinDepression =
                (1.0f - smoothStep(0.0f, 31.0f, basinRadius)) * 7.4f;
            const float basinRim = std::exp(-std::pow((basinRadius - 32.0f) / 6.5f, 2.0f)) *
                (2.0f + fbm(worldX * 0.07f, worldZ * 0.07f, settings.seed + 1501u) * 0.45f);

            const unsigned int index = z * n + x;
            heightSamples[index] = settings.baseHeight + rollingGround +
                                   h01 * settings.mountainHeight + surroundingHeight -
                                   basinDepression + basinRim;
        }
    }

    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(n * n);
    std::vector<unsigned char> blendPixels(n * n);
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
            // World-scale UVs repeat the 2K sphere material instead of stretching
            // a single copy over the 300 x 300 terrain.
            const glm::vec2 uv((static_cast<float>(x) * spacing) / 4.0f,
                               (static_cast<float>(z) * spacing) / 4.0f);
            const glm::vec2 blendUv(static_cast<float>(x) / static_cast<float>(n - 1),
                                    static_cast<float>(z) / static_cast<float>(n - 1));
            const glm::vec3 position(-settings.size * 0.5f + static_cast<float>(x) * spacing,
                                     heightSamples[i],
                                     -18.0f - settings.size * 0.5f + static_cast<float>(z) * spacing);
            vertices.push_back({position, normal, uv, blendUv, tangent, bitangent});

            // This matches the vegetation biome (low + gentle terrain), but uses
            // wide smooth bands and noise so rock and grass never meet as a hard line.
            const float heightSuitability = 1.0f - smoothStep(4.8f, 7.2f, heightSamples[i]);
            const float slopeSuitability = smoothStep(0.80f, 0.94f, normal.y);
            const float transitionNoise =
                fbm(position.x * 0.032f, position.z * 0.032f, settings.seed + 1201u) * 0.22f +
                fbm(position.x * 0.085f, position.z * 0.085f, settings.seed + 1301u) * 0.08f;
            const float basinDistance = std::sqrt(position.x * position.x +
                ((position.z + 18.0f) / 0.84f) * ((position.z + 18.0f) / 0.84f));
            const float dryLandMask = smoothStep(25.0f, 36.0f, basinDistance);
            // Material biome is altitude-driven: all gentle lowland receives the
            // Grass005 PBR set, while steep/high terrain resolves to Rock060.
            const float grassWeight = smoothStep(0.12f, 0.78f,
                heightSuitability * slopeSuitability * dryLandMask +
                transitionNoise * dryLandMask * 0.55f);
            blendPixels[i] = static_cast<unsigned char>(glm::clamp(grassWeight, 0.0f, 1.0f) * 255.0f);
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
    if (terrainMaterial.heightTex)
        textures.push_back({terrainMaterial.heightTex->getID(), "texture_height", "terrain://rock/height"});

    glGenTextures(1, &blendMaskTexture);
    glBindTexture(GL_TEXTURE_2D, blendMaskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(n), static_cast<GLsizei>(n),
                 0, GL_RED, GL_UNSIGNED_BYTE, blendPixels.data());
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
    mesh = std::make_unique<Mesh>(std::move(vertices), std::move(indices),
                                  std::move(textures), material, glm::mat4(1.0f));
}
