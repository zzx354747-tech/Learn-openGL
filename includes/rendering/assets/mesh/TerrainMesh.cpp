#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace
{
constexpr std::uint32_t CacheMagic = 0x4D445441u; // ATDM
constexpr std::uint32_t CacheVersion = 9u;
constexpr float Pi = 3.14159265358979323846f;
constexpr std::size_t MaximumWaterRegions = 8u;

struct CachedLakeRegion
{
    float boundsXZ[4];
    float waterLevel;
    float area;
    float maximumDepth;
    float padding;
};

struct CacheHeader
{
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t resolution;
    std::uint32_t lakeDataResolution;
    std::uint64_t parameterHash;
    float waterLevel;
    float lakeBoundsXZ[4];
    float lakeArea;
    float maximumWaterDepth;
    std::uint32_t lakeSampleCount;
    std::uint32_t lakeCount;
    CachedLakeRegion lakes[MaximumWaterRegions];
};

float smoothStep(float a, float b, float x)
{
    if (a == b)
        return x < a ? 0.0f : 1.0f;
    const float t = glm::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

std::uint32_t hash2(int x, int y, std::uint32_t seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * 0x8da6b343u;
    h ^= static_cast<std::uint32_t>(y) * 0xd8163841u;
    h ^= seed * 0xcb1ab31fu;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    return h ^ (h >> 16);
}

float gradientNoise(float x, float y, std::uint32_t seed)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const auto fade = [](float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    };
    const auto dotGradient = [seed](int ix, int iy, float dx, float dy)
    {
        const float angle = static_cast<float>(hash2(ix, iy, seed) & 65535u) *
                            (2.0f * Pi / 65536.0f);
        return std::cos(angle) * dx + std::sin(angle) * dy;
    };
    const float a = dotGradient(x0, y0, fx, fy);
    const float b = dotGradient(x0 + 1, y0, fx - 1.0f, fy);
    const float c = dotGradient(x0, y0 + 1, fx, fy - 1.0f);
    const float d = dotGradient(x0 + 1, y0 + 1, fx - 1.0f, fy - 1.0f);
    return glm::mix(glm::mix(a, b, fade(fx)),
                    glm::mix(c, d, fade(fx)), fade(fy)) * 1.41421356f;
}

float fbm(float x, float y, std::uint32_t seed, int octaves)
{
    float sum = 0.0f;
    float amplitude = 0.5f;
    float normalization = 0.0f;
    for (int octave = 0; octave < octaves; ++octave)
    {
        sum += gradientNoise(x, y, seed + static_cast<std::uint32_t>(octave) * 101u) * amplitude;
        normalization += amplitude;
        x *= 2.03f;
        y *= 2.03f;
        amplitude *= 0.5f;
    }
    return normalization > 0.0f ? sum / normalization : 0.0f;
}

float smoothMax(float a, float b, float k)
{
    if (k <= 0.0f)
        return std::max(a, b);
    const float h = std::max(k - std::abs(a - b), 0.0f) / k;
    return std::max(a, b) + h * h * k * 0.25f;
}

struct DesignedLake
{
    glm::vec2 center;
    glm::vec2 radii;
    float waterLevel;
    float maximumDepth;
    float rimHeight;
    float bankWidth;
    float rotation;
    std::uint32_t seed;
};

float designedLakeSignedDistance(const glm::vec2& worldXZ,
                                 const DesignedLake& lake)
{
    const glm::vec2 worldDelta = worldXZ - lake.center;
    const float c = std::cos(lake.rotation);
    const float s = std::sin(lake.rotation);
    const glm::vec2 delta(c * worldDelta.x + s * worldDelta.y,
                         -s * worldDelta.x + c * worldDelta.y);

    // Very elongated authored water bodies are river reaches rather than
    // elliptical lakes. Bend their local centre line before evaluating the
    // same signed-distance profile so banks, depth and the LDM all agree on a
    // continuous winding channel. Taper the bend at both ends to keep the
    // authored bounds stable and avoid abrupt terminal hooks.
    glm::vec2 shapedDelta = delta;
    if (lake.radii.x > lake.radii.y * 4.0f)
    {
        const float along = glm::clamp(delta.x / lake.radii.x, -1.0f, 1.0f);
        const float endTaper = 1.0f - smoothStep(
            0.72f, 1.0f, std::abs(along));
        const float phase = static_cast<float>(lake.seed & 1023u) *
            (2.0f * Pi / 1024.0f);
        shapedDelta.y -= std::sin(along * Pi * 2.25f + phase) *
            lake.radii.y * 1.15f * endTaper;
    }
    const glm::vec2 normalized = shapedDelta / lake.radii;
    const float angle = std::atan2(normalized.y, normalized.x);
    const float outline = 1.0f +
        0.080f * std::sin(angle * 3.0f + 0.7f) +
        0.048f * std::sin(angle * 5.0f - 1.9f) +
        0.025f * std::sin(angle * 9.0f + 2.4f) +
        0.025f * gradientNoise(worldXZ.x * 0.0045f + 13.1f,
                               worldXZ.y * 0.0045f - 7.4f, lake.seed);
    const float normalizedRadius = glm::length(normalized);
    const glm::vec2 direction = normalizedRadius > 0.0001f
        ? normalized / normalizedRadius : glm::vec2(1.0f, 0.0f);
    const float metresPerNormalizedUnit = 1.0f / std::sqrt(
        direction.x * direction.x / (lake.radii.x * lake.radii.x) +
        direction.y * direction.y / (lake.radii.y * lake.radii.y));
    return (outline - normalizedRadius) * metresPerNormalizedUnit;
}

float designedLakeDepth(float signedDistance, const DesignedLake& lake)
{
    const float depthCoordinate = glm::clamp(
        signedDistance / (0.82f * std::min(lake.radii.x, lake.radii.y)),
        0.0f, 1.0f);
    return lake.maximumDepth * smoothStep(0.0f, 1.0f, depthCoordinate);
}

float sculptDesignedLake(float naturalHeight, const glm::vec2& worldXZ,
                         const DesignedLake& lake)
{
    const float shoreDistance = designedLakeSignedDistance(worldXZ, lake);
    if (shoreDistance <= -lake.bankWidth)
        return naturalHeight;

    if (shoreDistance >= 0.0f)
        return lake.waterLevel - designedLakeDepth(shoreDistance, lake);

    const float outside = glm::clamp(-shoreDistance / lake.bankWidth,
                                     0.0f, 1.0f);
    const float authoredBank = lake.waterLevel + (-shoreDistance) * 0.34f +
        lake.rimHeight * std::sin(outside * Pi);
    const float influence = 1.0f - smoothStep(0.0f, 1.0f, outside);
    return glm::mix(naturalHeight, authoredBank, influence);
}

std::array<DesignedLake, 7> makeDesignedLakes(
    const TerrainMesh::Settings& settings)
{
    return {{
        {settings.lakeCenter, settings.lakeRadii, settings.lakeWaterLevel,
         settings.lakeBasinDepth, settings.lakeRimHeight,
         settings.lakeBankWidth, 0.0f, settings.seed + 1709u},
        {settings.meadowLakeCenter, settings.meadowLakeRadii,
         settings.meadowLakeWaterLevel, settings.meadowLakeBasinDepth,
         settings.meadowLakeRimHeight, settings.meadowLakeBankWidth,
         0.0f, settings.seed + 3911u},

        // Additional grassland tarns (haizi).
        {glm::vec2(-3060.0f, 1510.0f), glm::vec2(205.0f, 125.0f),
         58.0f, 18.0f, 8.0f, 58.0f, -0.18f, settings.seed + 5101u},
        {glm::vec2(-1540.0f, 2110.0f), glm::vec2(255.0f, 150.0f),
         108.0f, 22.0f, 10.0f, 66.0f, 0.26f, settings.seed + 6113u},

        // Long narrow authored water bodies read as winding river reaches at
        // landscape distance while retaining the same LDM/water renderer.
        {glm::vec2(-3190.0f, 430.0f), glm::vec2(660.0f, 54.0f),
         46.0f, 8.0f, 3.0f, 34.0f, 0.22f, settings.seed + 7121u},
        {glm::vec2(-1510.0f, 520.0f), glm::vec2(720.0f, 48.0f),
         64.0f, 7.0f, 3.0f, 32.0f, -0.34f, settings.seed + 8111u},
        {glm::vec2(-760.0f, 1760.0f), glm::vec2(590.0f, 44.0f),
         96.0f, 7.0f, 3.0f, 30.0f, 0.42f, settings.seed + 9109u}
    }};
}

glm::vec2 bezier(const TerrainMesh::Settings& settings, float t)
{
    const float u = 1.0f - t;
    return u * u * settings.ridgeP0 + 2.0f * u * t * settings.ridgeP1 +
           t * t * settings.ridgeP2;
}

void hashBytes(std::uint64_t& hash, const void* data, std::size_t size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
}

std::uint16_t floatToHalf(float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const std::uint32_t sign = (bits >> 16u) & 0x8000u;
    int exponent = static_cast<int>((bits >> 23u) & 0xffu) - 127 + 15;
    std::uint32_t mantissa = bits & 0x7fffffu;
    if (exponent <= 0)
    {
        if (exponent < -10)
            return static_cast<std::uint16_t>(sign);
        mantissa = (mantissa | 0x800000u) >> static_cast<unsigned int>(1 - exponent);
        return static_cast<std::uint16_t>(sign | ((mantissa + 0x1000u) >> 13u));
    }
    if (exponent >= 31)
        return static_cast<std::uint16_t>(sign | 0x7c00u);
    return static_cast<std::uint16_t>(sign |
        (static_cast<std::uint32_t>(exponent) << 10u) |
        ((mantissa + 0x1000u) >> 13u));
}

float halfToFloat(std::uint16_t value)
{
    const std::uint32_t sign = static_cast<std::uint32_t>(value & 0x8000u) << 16u;
    const std::uint32_t encodedExponent = (value >> 10u) & 0x1fu;
    std::uint32_t mantissa = value & 0x03ffu;
    std::uint32_t bits = 0;
    if (encodedExponent == 0)
    {
        if (mantissa == 0)
            bits = sign;
        else
        {
            int exponent = -14;
            while ((mantissa & 0x0400u) == 0)
            {
                mantissa <<= 1u;
                --exponent;
            }
            mantissa &= 0x03ffu;
            bits = sign | (static_cast<std::uint32_t>(exponent + 127) << 23u) |
                   (mantissa << 13u);
        }
    }
    else if (encodedExponent == 31u)
        bits = sign | 0x7f800000u | (mantissa << 13u);
    else
        bits = sign | ((encodedExponent + 112u) << 23u) | (mantissa << 13u);
    float result = 0.0f;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::filesystem::path cacheDirectory()
{
#ifdef OPENGL_PROJECT_ROOT
    return std::filesystem::path(OPENGL_PROJECT_ROOT) / "build" / "terrain_cache";
#else
    return std::filesystem::path("../build/terrain_cache");
#endif
}
}

TerrainMesh::TerrainMesh()
    : TerrainMesh(Settings{})
{
}

TerrainMesh::TerrainMesh(const Settings& nextSettings)
    : settings(nextSettings)
{
    settings.resolution = std::max(settings.resolution, 16u);
    settings.lakeDataResolution = std::max(settings.lakeDataResolution, 64u);
    settings.meshResolution = std::max(settings.meshResolution, 3u);
    settings.ridgedOctaves = std::clamp(settings.ridgedOctaves, 1, 12);

    rockMaterial = Material::loadFromDirectory("../textures/PBR/Rock060_2K-PNG");
    grassMaterial = Material::loadFromDirectory("../textures/PBR/Grass005_2K-PNG");
    snowMaterial.albedoTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Color.png");
    snowMaterial.normalTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_NormalGL.png");
    snowMaterial.roughnessTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Roughness.png");
    snowMaterial.heightTex.emplace(
        "../textures/PBR/Snow010A_1K-PNG/Snow010A_1K-PNG_Displacement.png");
    generate();
}

TerrainMesh::~TerrainMesh()
{
    if (terrainDataTexture)
        glDeleteTextures(1, &terrainDataTexture);
    if (lakeDataTexture)
        glDeleteTextures(1, &lakeDataTexture);
    if (detailNoiseTexture)
        glDeleteTextures(1, &detailNoiseTexture);
}

std::uint64_t TerrainMesh::parameterHash() const
{
    std::uint64_t hash = 1469598103934665603ull;
    hashBytes(hash, &CacheVersion, sizeof(CacheVersion));
    hashBytes(hash, &settings.resolution, sizeof(settings.resolution));
    hashBytes(hash, &settings.lakeDataResolution, sizeof(settings.lakeDataResolution));
    hashBytes(hash, &settings.meshResolution, sizeof(settings.meshResolution));
    hashBytes(hash, &settings.size, sizeof(settings.size));
    hashBytes(hash, &settings.mountainHeight, sizeof(settings.mountainHeight));
    hashBytes(hash, &settings.baseHeight, sizeof(settings.baseHeight));
    hashBytes(hash, &settings.seed, sizeof(settings.seed));
    hashBytes(hash, &settings.ridgeP0, sizeof(settings.ridgeP0));
    hashBytes(hash, &settings.ridgeP1, sizeof(settings.ridgeP1));
    hashBytes(hash, &settings.ridgeP2, sizeof(settings.ridgeP2));
    hashBytes(hash, settings.peaks.data(), sizeof(settings.peaks));
    hashBytes(hash, &settings.smoothMaxK, sizeof(settings.smoothMaxK));
    hashBytes(hash, &settings.baseContribution, sizeof(settings.baseContribution));
    hashBytes(hash, &settings.ridgedOctaves, sizeof(settings.ridgedOctaves));
    hashBytes(hash, &settings.ridgedContribution, sizeof(settings.ridgedContribution));
    hashBytes(hash, &settings.ridgedGain, sizeof(settings.ridgedGain));
    hashBytes(hash, &settings.lacunarity, sizeof(settings.lacunarity));
    hashBytes(hash, &settings.persistence, sizeof(settings.persistence));
    hashBytes(hash, &settings.detailContribution, sizeof(settings.detailContribution));
    hashBytes(hash, &settings.warpStrength, sizeof(settings.warpStrength));
    hashBytes(hash, &settings.upliftExponent, sizeof(settings.upliftExponent));
    hashBytes(hash, &settings.edgeFade, sizeof(settings.edgeFade));
    hashBytes(hash, &settings.curvatureRange, sizeof(settings.curvatureRange));
    hashBytes(hash, &settings.lakeCenter, sizeof(settings.lakeCenter));
    hashBytes(hash, &settings.lakeRadii, sizeof(settings.lakeRadii));
    hashBytes(hash, &settings.lakeWaterLevel, sizeof(settings.lakeWaterLevel));
    hashBytes(hash, &settings.lakeBasinDepth, sizeof(settings.lakeBasinDepth));
    hashBytes(hash, &settings.lakeRimHeight, sizeof(settings.lakeRimHeight));
    hashBytes(hash, &settings.lakeBankWidth, sizeof(settings.lakeBankWidth));
    hashBytes(hash, &settings.meadowLakeCenter, sizeof(settings.meadowLakeCenter));
    hashBytes(hash, &settings.meadowLakeRadii, sizeof(settings.meadowLakeRadii));
    hashBytes(hash, &settings.meadowLakeWaterLevel, sizeof(settings.meadowLakeWaterLevel));
    hashBytes(hash, &settings.meadowLakeBasinDepth, sizeof(settings.meadowLakeBasinDepth));
    hashBytes(hash, &settings.meadowLakeRimHeight, sizeof(settings.meadowLakeRimHeight));
    hashBytes(hash, &settings.meadowLakeBankWidth, sizeof(settings.meadowLakeBankWidth));
    return hash;
}

bool TerrainMesh::loadCache()
{
    const std::filesystem::path path = cacheDirectory() /
        ("alpine_" + std::to_string(parameterHash()) + ".bin");
    std::ifstream stream(path, std::ios::binary);
    CacheHeader header{};
    if (!stream.read(reinterpret_cast<char*>(&header), sizeof(header)) ||
        header.magic != CacheMagic || header.version != CacheVersion ||
        header.resolution != settings.resolution ||
        header.lakeDataResolution != settings.lakeDataResolution ||
        header.parameterHash != parameterHash())
    {
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(settings.resolution) *
                              settings.resolution;
    heightSamples.resize(count);
    terrainDataHalf.resize(count * 4u);
    const std::size_t lakeCount =
        static_cast<std::size_t>(settings.lakeDataResolution) *
        settings.lakeDataResolution;
    lakeDataHalf.resize(lakeCount * 3u);
    if (!stream.read(reinterpret_cast<char*>(heightSamples.data()),
                     static_cast<std::streamsize>(count * sizeof(float))) ||
        !stream.read(reinterpret_cast<char*>(terrainDataHalf.data()),
                     static_cast<std::streamsize>(count * 4u * sizeof(std::uint16_t))) ||
        !stream.read(reinterpret_cast<char*>(lakeDataHalf.data()),
                     static_cast<std::streamsize>(lakeCount * 3u * sizeof(std::uint16_t))))
    {
        heightSamples.clear();
        terrainDataHalf.clear();
        lakeDataHalf.clear();
        return false;
    }
    waterLevel = header.waterLevel;
    lakeBoundsXZ = glm::vec4(header.lakeBoundsXZ[0], header.lakeBoundsXZ[1],
                             header.lakeBoundsXZ[2], header.lakeBoundsXZ[3]);
    lakeArea = header.lakeArea;
    maximumWaterDepth = header.maximumWaterDepth;
    lakeRegions.clear();
    if (header.lakeCount == 0u || header.lakeCount > MaximumWaterRegions)
        return false;
    for (std::uint32_t i = 0; i < header.lakeCount; ++i)
    {
        const CachedLakeRegion& cached = header.lakes[i];
        LakeRegion region;
        region.boundsXZ = glm::vec4(cached.boundsXZ[0], cached.boundsXZ[1],
                                    cached.boundsXZ[2], cached.boundsXZ[3]);
        region.waterLevel = cached.waterLevel;
        region.area = cached.area;
        region.maximumDepth = cached.maximumDepth;
        lakeRegions.push_back(region);
    }
    return true;
}

void TerrainMesh::saveCache() const
{
    std::error_code error;
    std::filesystem::create_directories(cacheDirectory(), error);
    if (error)
        return;
    const std::filesystem::path finalPath = cacheDirectory() /
        ("alpine_" + std::to_string(parameterHash()) + ".bin");
    const std::filesystem::path temporaryPath = finalPath.string() + ".tmp";
    std::ofstream stream(temporaryPath, std::ios::binary | std::ios::trunc);
    CacheHeader header{};
    header.magic = CacheMagic;
    header.version = CacheVersion;
    header.resolution = settings.resolution;
    header.lakeDataResolution = settings.lakeDataResolution;
    header.parameterHash = parameterHash();
    header.waterLevel = waterLevel;
    header.lakeBoundsXZ[0] = lakeBoundsXZ.x;
    header.lakeBoundsXZ[1] = lakeBoundsXZ.y;
    header.lakeBoundsXZ[2] = lakeBoundsXZ.z;
    header.lakeBoundsXZ[3] = lakeBoundsXZ.w;
    header.lakeArea = lakeArea;
    header.maximumWaterDepth = maximumWaterDepth;
    header.lakeSampleCount = static_cast<std::uint32_t>(lakeDataHalf.size() / 3u);
    header.lakeCount = static_cast<std::uint32_t>(std::min<std::size_t>(
        lakeRegions.size(), MaximumWaterRegions));
    for (std::uint32_t i = 0; i < header.lakeCount; ++i)
    {
        const LakeRegion& region = lakeRegions[i];
        CachedLakeRegion& cached = header.lakes[i];
        cached.boundsXZ[0] = region.boundsXZ.x;
        cached.boundsXZ[1] = region.boundsXZ.y;
        cached.boundsXZ[2] = region.boundsXZ.z;
        cached.boundsXZ[3] = region.boundsXZ.w;
        cached.waterLevel = region.waterLevel;
        cached.area = region.area;
        cached.maximumDepth = region.maximumDepth;
    }
    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    stream.write(reinterpret_cast<const char*>(heightSamples.data()),
                 static_cast<std::streamsize>(heightSamples.size() * sizeof(float)));
    stream.write(reinterpret_cast<const char*>(terrainDataHalf.data()),
                 static_cast<std::streamsize>(terrainDataHalf.size() * sizeof(std::uint16_t)));
    stream.write(reinterpret_cast<const char*>(lakeDataHalf.data()),
                 static_cast<std::streamsize>(lakeDataHalf.size() * sizeof(std::uint16_t)));
    stream.close();
    if (!stream)
        return;
    std::filesystem::remove(finalPath, error);
    error.clear();
    std::filesystem::rename(temporaryPath, finalPath, error);
}

void TerrainMesh::generate()
{
    const auto start = std::chrono::steady_clock::now();
    cacheHit = loadCache();
    if (!cacheHit)
    {
        generateHeightField();
        generateLakeData();
        computeDerivedFields();
        saveCache();
    }
    uploadTerrainTextures();
    buildMesh();
    const double milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "Terrain: " << (cacheHit ? "cache hit" : "generated")
              << ", ready in " << milliseconds << " ms" << std::endl;
    for (std::size_t i = 0; i < lakeRegions.size(); ++i)
    {
        const LakeRegion& lake = lakeRegions[i];
        std::cout << "Lake[" << i << "]: level=" << lake.waterLevel
                  << " m, area=" << lake.area
                  << " m^2, maxDepth=" << lake.maximumDepth
                  << " m, boundsXZ=(" << lake.boundsXZ.x << ", " << lake.boundsXZ.y
                  << ")-(" << lake.boundsXZ.z << ", " << lake.boundsXZ.w << ")"
                  << std::endl;
    }
}

void TerrainMesh::generateHeightField()
{
    const unsigned int n = settings.resolution;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const auto designedLakes = makeDesignedLakes(settings);
    heightSamples.assign(static_cast<std::size_t>(n) * n, settings.baseHeight);
    const unsigned int workerCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int worker = 0; worker < workerCount; ++worker)
    {
        const unsigned int firstRow = n * worker / workerCount;
        const unsigned int lastRow = n * (worker + 1u) / workerCount;
        workers.emplace_back([this, n, spacing, firstRow, lastRow, designedLakes]()
        {
            for (unsigned int z = firstRow; z < lastRow; ++z)
            {
                for (unsigned int x = 0; x < n; ++x)
                {
                    const float worldX = -settings.size * 0.5f + x * spacing;
                    const float worldZ = -settings.size * 0.5f + z * spacing;
                    const float warpX = fbm(worldX * 0.00045f + 7.1f,
                                            worldZ * 0.00045f - 3.7f,
                                            settings.seed + 11u, 4) * settings.warpStrength;
                    const float warpZ = fbm(worldX * 0.00045f - 9.4f,
                                            worldZ * 0.00045f + 5.2f,
                                            settings.seed + 29u, 4) * settings.warpStrength;
                    const glm::vec2 p(worldX + warpX, worldZ + warpZ);

                    float base = 0.0f;
                    for (const Peak& peak : settings.peaks)
                    {
                        const glm::vec2 center = bezier(settings, peak.bezierT);
                        const glm::vec2 delta = p - center;
                        const float gaussian = peak.amplitude * std::exp(
                            -glm::dot(delta, delta) /
                            (2.0f * peak.sigma * peak.sigma));
                        base = smoothMax(base, gaussian, settings.smoothMaxK);
                    }
                    base = glm::clamp(base, 0.0f, 1.0f);

                    float frequency = 0.00145f;
                    float amplitude = 1.0f;
                    float weight = 1.0f;
                    float ridgeSum = 0.0f;
                    float ridgeNorm = 0.0f;
                    for (int octave = 0; octave < settings.ridgedOctaves; ++octave)
                    {
                        float ridge = 1.0f - std::abs(gradientNoise(
                            p.x * frequency, p.y * frequency,
                            settings.seed + 211u + static_cast<unsigned int>(octave) * 67u));
                        ridge = ridge * ridge;
                        ridgeSum += ridge * amplitude * weight;
                        ridgeNorm += amplitude;
                        weight = glm::clamp(ridge * settings.ridgedGain, 0.0f, 1.0f);
                        frequency *= settings.lacunarity;
                        amplitude *= settings.persistence;
                    }
                    const float ridged = ridgeNorm > 0.0f ? ridgeSum / ridgeNorm : 0.0f;
                    const float ridgeMask = smoothStep(0.20f, 0.60f, base);
                    const float detail = fbm(p.x * 0.018f, p.y * 0.018f,
                                             settings.seed + 701u, 3);
                    float height01 = base * settings.baseContribution +
                        ridged * ridgeMask * settings.ridgedContribution +
                        detail * settings.detailContribution;
                    height01 = std::pow(glm::clamp(height01, 0.0f, 1.0f),
                                        settings.upliftExponent);

                    if (settings.edgeFade)
                    {
                        const float edge = std::min(
                            settings.size * 0.5f - std::abs(worldX),
                            settings.size * 0.5f - std::abs(worldZ));
                        height01 *= smoothStep(0.0f, settings.size * 0.10f, edge);
                    }

                    // Lake floors and banks are authored in the terrain itself.
                    // The LDM later evaluates these exact same lake functions.
                    float finalHeight = settings.baseHeight +
                                        height01 * settings.mountainHeight;
                    const glm::vec2 worldXZ(worldX, worldZ);
                    for (const DesignedLake& lake : designedLakes)
                        finalHeight = sculptDesignedLake(finalHeight, worldXZ, lake);
                    heightSamples[static_cast<std::size_t>(z) * n + x] = finalHeight;
                }
            }
        });
    }
    for (std::thread& worker : workers)
        worker.join();
}

void TerrainMesh::generateLakeData()
{
    const unsigned int lakeResolution = settings.lakeDataResolution;
    const std::size_t lakeSampleCount =
        static_cast<std::size_t>(lakeResolution) * lakeResolution;
    const float lakeSpacing = settings.size /
                              static_cast<float>(lakeResolution - 1u);
    const float lakeWorldMinimum = -settings.size * 0.5f;
    const auto designedLakes = makeDesignedLakes(settings);
    lakeDataHalf.assign(lakeSampleCount * 3u, floatToHalf(0.0f));
    lakeRegions.clear();
    lakeArea = 0.0f;
    maximumWaterDepth = 0.0f;
    glm::vec2 authoredBoundsMin(std::numeric_limits<float>::max());
    glm::vec2 authoredBoundsMax(-std::numeric_limits<float>::max());

    for (const DesignedLake& lake : designedLakes)
    {
        std::size_t wetCount = 0u;
        float localMaximumDepth = 0.0f;
        glm::vec2 boundsMin(std::numeric_limits<float>::max());
        glm::vec2 boundsMax(-std::numeric_limits<float>::max());
        for (unsigned int z = 0; z < lakeResolution; ++z)
        {
            const float worldZ = lakeWorldMinimum + z * lakeSpacing;
            for (unsigned int x = 0; x < lakeResolution; ++x)
            {
                const glm::vec2 worldXZ(
                    lakeWorldMinimum + x * lakeSpacing, worldZ);
                const float signedDistance =
                    designedLakeSignedDistance(worldXZ, lake);
                if (signedDistance <= 0.0f)
                    continue;

                const float depth = designedLakeDepth(signedDistance, lake);
                const std::size_t channel =
                    (static_cast<std::size_t>(z) * lakeResolution + x) * 3u;
                if (halfToFloat(lakeDataHalf[channel + 2u]) != 0.0f)
                    throw std::runtime_error("Authored lake regions overlap");
                lakeDataHalf[channel] = floatToHalf(depth);
                lakeDataHalf[channel + 1u] = floatToHalf(
                    glm::clamp(signedDistance, 0.0f, 128.0f));
                lakeDataHalf[channel + 2u] = floatToHalf(lake.waterLevel);
                boundsMin = glm::min(boundsMin, worldXZ);
                boundsMax = glm::max(boundsMax, worldXZ);
                localMaximumDepth = std::max(localMaximumDepth, depth);
                ++wetCount;
            }
        }

        if (wetCount == 0u)
            throw std::runtime_error("Authored lake contains no samples");
        const float localArea = static_cast<float>(wetCount) *
                                lakeSpacing * lakeSpacing;
        LakeRegion region;
        region.boundsXZ = glm::vec4(boundsMin.x - lakeSpacing * 2.0f,
                                    boundsMin.y - lakeSpacing * 2.0f,
                                    boundsMax.x + lakeSpacing * 2.0f,
                                    boundsMax.y + lakeSpacing * 2.0f);
        region.waterLevel = lake.waterLevel;
        region.area = localArea;
        region.maximumDepth = localMaximumDepth;
        lakeRegions.push_back(region);
        lakeArea += localArea;
        maximumWaterDepth = std::max(maximumWaterDepth, localMaximumDepth);
        authoredBoundsMin = glm::min(authoredBoundsMin, boundsMin);
        authoredBoundsMax = glm::max(authoredBoundsMax, boundsMax);
    }

    // Outside samples also store the analytic signed distance. This gives the
    // fragment shader a continuous, linearly filtered edge instead of a
    // flood-fill/distance-transform staircase.
    for (unsigned int z = 0; z < lakeResolution; ++z)
    {
        const float worldZ = lakeWorldMinimum + z * lakeSpacing;
        for (unsigned int x = 0; x < lakeResolution; ++x)
        {
            const std::size_t channel =
                (static_cast<std::size_t>(z) * lakeResolution + x) * 3u;
            if (halfToFloat(lakeDataHalf[channel + 2u]) != 0.0f)
                continue;
            const glm::vec2 worldXZ(
                lakeWorldMinimum + x * lakeSpacing, worldZ);
            float signedDistance = -std::numeric_limits<float>::max();
            for (const DesignedLake& lake : designedLakes)
                signedDistance = std::max(
                    signedDistance, designedLakeSignedDistance(worldXZ, lake));
            lakeDataHalf[channel + 1u] = floatToHalf(
                glm::clamp(signedDistance, -128.0f, 0.0f));
        }
    }

    waterLevel = lakeRegions.front().waterLevel;
    lakeBoundsXZ = glm::vec4(authoredBoundsMin.x - lakeSpacing * 2.0f,
                             authoredBoundsMin.y - lakeSpacing * 2.0f,
                             authoredBoundsMax.x + lakeSpacing * 2.0f,
                             authoredBoundsMax.y + lakeSpacing * 2.0f);
}

void TerrainMesh::computeDerivedFields()
{
    const unsigned int n = settings.resolution;
    const std::size_t count = static_cast<std::size_t>(n) * n;
    const float step = settings.size / static_cast<float>(n - 1u);
    terrainData.assign(count * 4u, 0.0f);
    std::vector<float> blurred(count);
    constexpr float kernel[3] = {1.0f, 2.0f, 1.0f};
    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            float sum = 0.0f;
            float total = 0.0f;
            for (int oz = -1; oz <= 1; ++oz)
            {
                const unsigned int sz = static_cast<unsigned int>(std::clamp(
                    static_cast<int>(z) + oz, 0, static_cast<int>(n) - 1));
                for (int ox = -1; ox <= 1; ++ox)
                {
                    const unsigned int sx = static_cast<unsigned int>(std::clamp(
                        static_cast<int>(x) + ox, 0, static_cast<int>(n) - 1));
                    const float w = kernel[ox + 1] * kernel[oz + 1];
                    sum += heightSamples[static_cast<std::size_t>(sz) * n + sx] * w;
                    total += w;
                }
            }
            blurred[static_cast<std::size_t>(z) * n + x] = sum / total;
        }
    }

    const unsigned int workerCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    for (unsigned int worker = 0; worker < workerCount; ++worker)
    {
        const unsigned int firstRow = n * worker / workerCount;
        const unsigned int lastRow = n * (worker + 1u) / workerCount;
        workers.emplace_back([&, firstRow, lastRow]()
        {
            for (unsigned int z = firstRow; z < lastRow; ++z)
            {
                const unsigned int zd = z > 0 ? z - 1u : z;
                const unsigned int zu = z + 1u < n ? z + 1u : z;
                for (unsigned int x = 0; x < n; ++x)
                {
                    const unsigned int xl = x > 0 ? x - 1u : x;
                    const unsigned int xr = x + 1u < n ? x + 1u : x;
                    const std::size_t i = static_cast<std::size_t>(z) * n + x;
                    const float dxDenom = std::max(static_cast<float>(xr - xl) * step, 0.001f);
                    const float dzDenom = std::max(static_cast<float>(zu - zd) * step, 0.001f);
                    const float dhdx = (heightSamples[static_cast<std::size_t>(z) * n + xr] -
                                        heightSamples[static_cast<std::size_t>(z) * n + xl]) / dxDenom;
                    const float dhdz = (heightSamples[static_cast<std::size_t>(zu) * n + x] -
                                        heightSamples[static_cast<std::size_t>(zd) * n + x]) / dzDenom;
                    const glm::vec3 normal = glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
                    const float slope = std::acos(glm::clamp(normal.y, 0.0f, 1.0f)) / (Pi * 0.5f);
                    const float gradientLength = std::sqrt(dhdx * dhdx + dhdz * dhdz);
                    const float aspect = gradientLength < 1e-5f
                        ? 0.5f
                        : (std::atan2(dhdz, dhdx) + Pi) / (2.0f * Pi);
                    const float center = blurred[i];
                    // World-space five-point Laplacian. Positive is concave.
                    const float laplacian = (
                        blurred[static_cast<std::size_t>(z) * n + xl] +
                        blurred[static_cast<std::size_t>(z) * n + xr] +
                        blurred[static_cast<std::size_t>(zd) * n + x] +
                        blurred[static_cast<std::size_t>(zu) * n + x] -
                        4.0f * center) / (step * step);
                    terrainData[i * 4u + 0u] = glm::clamp(
                        (heightSamples[i] - settings.baseHeight) / settings.mountainHeight,
                        0.0f, 1.0f);
                    terrainData[i * 4u + 1u] = slope;
                    terrainData[i * 4u + 2u] = aspect;
                    terrainData[i * 4u + 3u] = glm::clamp(
                        0.5f + 0.5f * laplacian / settings.curvatureRange,
                        0.0f, 1.0f);
                }
            }
        });
    }
    for (std::thread& worker : workers)
        worker.join();
    terrainDataHalf.resize(terrainData.size());
    for (std::size_t i = 0; i < terrainData.size(); ++i)
        terrainDataHalf[i] = floatToHalf(terrainData[i]);
    terrainData.clear();
    terrainData.shrink_to_fit();
}

void TerrainMesh::uploadTerrainTextures()
{
    const GLsizei n = static_cast<GLsizei>(settings.resolution);
    glGenTextures(1, &terrainDataTexture);
    glBindTexture(GL_TEXTURE_2D, terrainDataTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, n, n, 0, GL_RGBA,
                 GL_HALF_FLOAT, terrainDataHalf.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &lakeDataTexture);
    glBindTexture(GL_TEXTURE_2D, lakeDataTexture);
    const GLsizei lakeN = static_cast<GLsizei>(settings.lakeDataResolution);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, lakeN, lakeN, 0, GL_RGB,
                 GL_HALF_FLOAT, lakeDataHalf.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    constexpr int NoiseSize = 256;
    std::vector<float> detailHeights(NoiseSize * NoiseSize);
    std::vector<unsigned char> noise(NoiseSize * NoiseSize * 4u);
    for (int y = 0; y < NoiseSize; ++y)
    {
        for (int x = 0; x < NoiseSize; ++x)
        {
            const std::size_t sampleIndex = static_cast<std::size_t>(y * NoiseSize + x);
            const std::size_t i = sampleIndex * 4u;
            const float macro = 0.5f + 0.5f * fbm(x / 42.0f, y / 42.0f,
                                                  settings.seed + 4001u, 4);
            const float detail = 0.5f + 0.5f * fbm(x / 9.0f, y / 9.0f,
                                                   settings.seed + 5003u, 3);
            detailHeights[sampleIndex] = detail;
            noise[i] = static_cast<unsigned char>(glm::clamp(macro, 0.0f, 1.0f) * 255.0f);
            noise[i + 1u] = static_cast<unsigned char>(glm::clamp(detail, 0.0f, 1.0f) * 255.0f);
        }
    }
    for (int y = 0; y < NoiseSize; ++y)
    {
        for (int x = 0; x < NoiseSize; ++x)
        {
            const int xl = (x + NoiseSize - 1) % NoiseSize;
            const int xr = (x + 1) % NoiseSize;
            const int yd = (y + NoiseSize - 1) % NoiseSize;
            const int yu = (y + 1) % NoiseSize;
            const float nx = glm::clamp(
                (detailHeights[static_cast<std::size_t>(y * NoiseSize + xl)] -
                 detailHeights[static_cast<std::size_t>(y * NoiseSize + xr)]) * 1.8f,
                -1.0f, 1.0f);
            const float nz = glm::clamp(
                (detailHeights[static_cast<std::size_t>(yd * NoiseSize + x)] -
                 detailHeights[static_cast<std::size_t>(yu * NoiseSize + x)]) * 1.8f,
                -1.0f, 1.0f);
            const std::size_t i = static_cast<std::size_t>(y * NoiseSize + x) * 4u;
            noise[i + 2u] = static_cast<unsigned char>((nx * 0.5f + 0.5f) * 255.0f);
            noise[i + 3u] = static_cast<unsigned char>((nz * 0.5f + 0.5f) * 255.0f);
        }
    }
    glGenTextures(1, &detailNoiseTexture);
    glBindTexture(GL_TEXTURE_2D, detailNoiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, NoiseSize, NoiseSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TerrainMesh::buildMesh()
{
    const unsigned int n = settings.meshResolution;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(n) * n);
    for (unsigned int z = 0; z < n; ++z)
    {
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = -settings.size * 0.5f + x * spacing;
            const float worldZ = -settings.size * 0.5f + z * spacing;
            const float height = sampleHeight(worldX, worldZ);
            const glm::vec3 normal = sampleNormal(worldX, worldZ);
            const float tangentStep = settings.size /
                                      static_cast<float>(settings.resolution - 1u);
            const float dhdx = (sampleHeight(worldX + tangentStep, worldZ) -
                                sampleHeight(worldX - tangentStep, worldZ)) /
                               (2.0f * tangentStep);
            const glm::vec3 tangent = glm::normalize(glm::vec3(1.0f, dhdx, 0.0f));
            const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));
            const glm::vec2 dataUv(static_cast<float>(x) / static_cast<float>(n - 1u),
                                   static_cast<float>(z) / static_cast<float>(n - 1u));
            vertices.push_back({glm::vec3(worldX, height, worldZ), normal,
                                glm::vec2(worldX, worldZ) / 64.0f,
                                dataUv, tangent, bitangent});
        }
    }
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<std::size_t>(n - 1u) * (n - 1u) * 6u);
    for (unsigned int z = 0; z + 1u < n; ++z)
    {
        for (unsigned int x = 0; x + 1u < n; ++x)
        {
            const unsigned int a = z * n + x;
            const unsigned int b = a + 1u;
            const unsigned int c = a + n;
            const unsigned int d = c + 1u;
            indices.insert(indices.end(), {a, c, b, b, c, d});
        }
    }

    std::vector<Texture> textures;
    const auto add = [&textures](const std::optional<GLTexture>& texture,
                                 const char* type, const char* path)
    {
        if (texture)
            textures.push_back({texture->getID(), type, path});
    };
    add(rockMaterial.albedoTex, "texture_baseColor", "terrain://rock/albedo");
    add(rockMaterial.normalTex, "texture_normal", "terrain://rock/normal");
    add(rockMaterial.roughnessTex, "texture_roughness", "terrain://rock/roughness");
    add(rockMaterial.heightTex, "texture_terrainRockHeight", "terrain://rock/height");
    textures.push_back({terrainDataTexture, "texture_terrainData", "terrain://tdm"});
    textures.push_back({detailNoiseTexture, "texture_terrainNoise", "terrain://detail-noise"});
    add(grassMaterial.albedoTex, "texture_terrainGrassAlbedo", "terrain://grass/albedo");
    add(grassMaterial.normalTex, "texture_terrainGrassNormal", "terrain://grass/normal");
    add(grassMaterial.roughnessTex, "texture_terrainGrassRoughness", "terrain://grass/roughness");
    add(grassMaterial.heightTex, "texture_terrainGrassHeight", "terrain://grass/height");
    add(snowMaterial.albedoTex, "texture_terrainSnowAlbedo", "terrain://snow/albedo");
    add(snowMaterial.normalTex, "texture_terrainSnowNormal", "terrain://snow/normal");
    add(snowMaterial.roughnessTex, "texture_terrainSnowRoughness", "terrain://snow/roughness");
    add(snowMaterial.heightTex, "texture_terrainSnowHeight", "terrain://snow/height");

    MaterialFlags material;
    material.baseColorFactor = glm::vec4(1.0f);
    material.roughnessFactor = 0.8f;
    material.metallicFactor = 0.0f;
    mesh = std::make_unique<Mesh>(std::move(vertices), std::move(indices),
                                  std::move(textures), material, glm::mat4(1.0f));
}

float TerrainMesh::sampleHeight(float worldX, float worldZ) const
{
    const unsigned int n = settings.resolution;
    if (heightSamples.empty())
        return settings.baseHeight;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f || gridX > n - 1.0f || gridZ > n - 1.0f)
        return settings.baseHeight;
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - x0;
    const float tz = gridZ - z0;
    const float a = glm::mix(heightSamples[static_cast<std::size_t>(z0) * n + x0],
                             heightSamples[static_cast<std::size_t>(z0) * n + x0 + 1u], tx);
    const float b = glm::mix(heightSamples[static_cast<std::size_t>(z0 + 1u) * n + x0],
                             heightSamples[static_cast<std::size_t>(z0 + 1u) * n + x0 + 1u], tx);
    return glm::mix(a, b, tz);
}

glm::vec3 TerrainMesh::sampleNormal(float worldX, float worldZ) const
{
    const float step = settings.size / static_cast<float>(settings.resolution - 1u);
    const float dx = (sampleHeight(worldX + step, worldZ) -
                      sampleHeight(worldX - step, worldZ)) / (2.0f * step);
    const float dz = (sampleHeight(worldX, worldZ + step) -
                      sampleHeight(worldX, worldZ - step)) / (2.0f * step);
    return glm::normalize(glm::vec3(-dx, 1.0f, -dz));
}

float TerrainMesh::sampleWorldHeight(float x, float z) const { return sampleHeight(x, z); }
glm::vec3 TerrainMesh::sampleWorldNormal(float x, float z) const { return sampleNormal(x, z); }

glm::vec4 TerrainMesh::sampleTerrainData(float worldX, float worldZ) const
{
    const unsigned int n = settings.resolution;
    if (terrainDataHalf.empty() || n < 2u)
        return glm::vec4(0.0f, 0.0f, 0.5f, 0.5f);
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f || gridX > n - 1.0f || gridZ > n - 1.0f)
        return glm::vec4(0.0f, 0.0f, 0.5f, 0.5f);
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const auto read = [this, n](unsigned int x, unsigned int z)
    {
        const std::size_t i = (static_cast<std::size_t>(z) * n + x) * 4u;
        return glm::vec4(halfToFloat(terrainDataHalf[i]),
                         halfToFloat(terrainDataHalf[i + 1u]),
                         halfToFloat(terrainDataHalf[i + 2u]),
                         halfToFloat(terrainDataHalf[i + 3u]));
    };
    const glm::vec4 a = glm::mix(read(x0, z0), read(x0 + 1u, z0), tx);
    const glm::vec4 b = glm::mix(read(x0, z0 + 1u), read(x0 + 1u, z0 + 1u), tx);
    return glm::mix(a, b, tz);
}

TerrainMesh::SurfaceSample TerrainMesh::sampleSurface(float worldX,
                                                      float worldZ) const
{
    SurfaceSample sample;
    // Vegetation and every other attached object must follow the triangles
    // that are actually rasterized, not the denser 1024^2 source heightfield.
    // The render mesh is deliberately coarser (256^2), so sampling the source
    // field directly can differ from the visible triangle by many metres on a
    // steep ridge. Reconstruct the exact a-c-b / b-c-d triangle used by
    // buildMesh() and evaluate its plane here.
    const unsigned int n = std::max(settings.meshResolution, 2u);
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = glm::clamp((worldX + settings.size * 0.5f) / spacing,
                                   0.0f, static_cast<float>(n - 1u));
    const float gridZ = glm::clamp((worldZ + settings.size * 0.5f) / spacing,
                                   0.0f, static_cast<float>(n - 1u));
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const float x = -settings.size * 0.5f + static_cast<float>(x0) * spacing;
    const float z = -settings.size * 0.5f + static_cast<float>(z0) * spacing;
    const float h00 = sampleHeight(x, z);
    const float h10 = sampleHeight(x + spacing, z);
    const float h01 = sampleHeight(x, z + spacing);
    const float h11 = sampleHeight(x + spacing, z + spacing);
    float renderedHeight = 0.0f;
    float dhdx = 0.0f;
    float dhdz = 0.0f;
    if (tx + tz <= 1.0f)
    {
        renderedHeight = h00 + tx * (h10 - h00) + tz * (h01 - h00);
        dhdx = (h10 - h00) / spacing;
        dhdz = (h01 - h00) / spacing;
    }
    else
    {
        renderedHeight = h10 * (1.0f - tz) + h01 * (1.0f - tx) +
                         h11 * (tx + tz - 1.0f);
        dhdx = (h11 - h01) / spacing;
        dhdz = (h11 - h10) / spacing;
    }
    sample.worldPosition = glm::vec3(worldX, renderedHeight, worldZ);
    sample.normal = glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
    sample.tdm = sampleTerrainData(worldX, worldZ);
    const glm::vec2 lake = sampleLakeData(worldX, worldZ);
    sample.waterDepth = lake.x;
    sample.signedDistanceToWater = lake.y;
    sample.underwater = lake.x > 0.01f && lake.y >= 0.0f;
    return sample;
}
bool TerrainMesh::isBelowWater(float x, float z) const { return isInsideLake(x, z); }

glm::vec2 TerrainMesh::sampleLakeData(float worldX, float worldZ) const
{
    const unsigned int n = settings.lakeDataResolution;
    if (lakeDataHalf.empty() || n < 2u)
        return glm::vec2(0.0f, -64.0f);
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f || gridX > n - 1.0f || gridZ > n - 1.0f)
        return glm::vec2(0.0f, -64.0f);
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - x0;
    const float tz = gridZ - z0;
    const auto read = [this, n](unsigned int x, unsigned int z)
    {
        const std::size_t i = (static_cast<std::size_t>(z) * n + x) * 3u;
        return glm::vec2(halfToFloat(lakeDataHalf[i]),
                         halfToFloat(lakeDataHalf[i + 1u]));
    };
    const glm::vec2 a = glm::mix(read(x0, z0), read(x0 + 1u, z0), tx);
    const glm::vec2 b = glm::mix(read(x0, z0 + 1u), read(x0 + 1u, z0 + 1u), tx);
    return glm::mix(a, b, tz);
}

float TerrainMesh::sampleWaterDepth(float x, float z) const
{
    return sampleLakeData(x, z).x;
}

float TerrainMesh::sampleSignedDistanceToWater(float x, float z) const
{
    return sampleLakeData(x, z).y;
}

bool TerrainMesh::isInsideLake(float x, float z) const
{
    const glm::vec2 data = sampleLakeData(x, z);
    return data.x > 0.01f && data.y >= 0.0f;
}

bool TerrainMesh::isInsideCentralTerrain(float x, float z) const
{
    return std::abs(x) <= settings.size * 0.5f &&
           std::abs(z) <= settings.size * 0.5f;
}

void TerrainMesh::draw(Shader& shader) const
{
    if (mesh)
        mesh->Draw(shader);
}

void TerrainMesh::updateStreaming(const glm::vec3&, const glm::mat4& viewProjection)
{
    const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0]);
    const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1]);
    const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2]);
    const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3]);
    frustumPlanes = {row3 + row0, row3 - row0, row3 + row1,
                     row3 - row1, row3 + row2, row3 - row2};
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
    if (!streamingValid)
        return true;
    const glm::vec3 center(0.0f, settings.mountainHeight * 0.4f, 0.0f);
    const float radius = std::sqrt(settings.size * settings.size * 0.5f +
                                   settings.mountainHeight * settings.mountainHeight);
    for (const glm::vec4& plane : frustumPlanes)
        if (glm::dot(glm::vec3(plane), center) + plane.w < -radius)
            return false;
    return true;
}
