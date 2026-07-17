#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <thread>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace
{
constexpr std::uint32_t CacheMagic = 0x4D445441u; // ATDM
constexpr std::uint32_t CacheVersion = 16u;
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
    if (terrainEnvironmentTexture)
        glDeleteTextures(1, &terrainEnvironmentTexture);
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
    hashBytes(hash, &settings.glacialValleyStrength, sizeof(settings.glacialValleyStrength));
    hashBytes(hash, &settings.riverStartArea, sizeof(settings.riverStartArea));
    hashBytes(hash, &settings.riverBaseWidth, sizeof(settings.riverBaseWidth));
    hashBytes(hash, &settings.riverWidthPerSqrtKm2, sizeof(settings.riverWidthPerSqrtKm2));
    hashBytes(hash, &settings.riverMaximumWidth, sizeof(settings.riverMaximumWidth));
    hashBytes(hash, &settings.riverBaseDepth, sizeof(settings.riverBaseDepth));
    hashBytes(hash, &settings.riverDepthPerSqrtKm2, sizeof(settings.riverDepthPerSqrtKm2));
    hashBytes(hash, &settings.minimumLakeArea, sizeof(settings.minimumLakeArea));
    hashBytes(hash, &settings.minimumLakeDepth, sizeof(settings.minimumLakeDepth));
    hashBytes(hash, &settings.lakeCenter, sizeof(settings.lakeCenter));
    hashBytes(hash, &settings.lakeRadii, sizeof(settings.lakeRadii));
    hashBytes(hash, &settings.lakeBasinDepth, sizeof(settings.lakeBasinDepth));
    hashBytes(hash, &settings.lakeRimHeight, sizeof(settings.lakeRimHeight));
    hashBytes(hash, &settings.meadowLakeCenter, sizeof(settings.meadowLakeCenter));
    hashBytes(hash, &settings.meadowLakeRadii, sizeof(settings.meadowLakeRadii));
    hashBytes(hash, &settings.meadowLakeBasinDepth, sizeof(settings.meadowLakeBasinDepth));
    hashBytes(hash, &settings.meadowLakeRimHeight, sizeof(settings.meadowLakeRimHeight));
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
    terrainEnvironmentHalf.resize(count * 2u);
    const std::size_t lakeCount =
        static_cast<std::size_t>(settings.lakeDataResolution) *
        settings.lakeDataResolution;
    lakeDataHalf.resize(lakeCount * 3u);
    if (!stream.read(reinterpret_cast<char*>(heightSamples.data()),
                     static_cast<std::streamsize>(count * sizeof(float))) ||
        !stream.read(reinterpret_cast<char*>(terrainDataHalf.data()),
                     static_cast<std::streamsize>(count * 4u * sizeof(std::uint16_t))) ||
        !stream.read(reinterpret_cast<char*>(terrainEnvironmentHalf.data()),
                     static_cast<std::streamsize>(count * 2u * sizeof(std::uint16_t))) ||
        !stream.read(reinterpret_cast<char*>(lakeDataHalf.data()),
                     static_cast<std::streamsize>(lakeCount * 3u * sizeof(std::uint16_t))))
    {
        heightSamples.clear();
        terrainDataHalf.clear();
        terrainEnvironmentHalf.clear();
        lakeDataHalf.clear();
        return false;
    }
    waterLevel = header.waterLevel;
    lakeBoundsXZ = glm::vec4(header.lakeBoundsXZ[0], header.lakeBoundsXZ[1],
                             header.lakeBoundsXZ[2], header.lakeBoundsXZ[3]);
    lakeArea = header.lakeArea;
    maximumWaterDepth = header.maximumWaterDepth;
    lakeRegions.clear();
    if (header.lakeCount > MaximumWaterRegions)
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
    stream.write(reinterpret_cast<const char*>(terrainEnvironmentHalf.data()),
                 static_cast<std::streamsize>(terrainEnvironmentHalf.size() * sizeof(std::uint16_t)));
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
        sculptGlacialLandforms();
        generateHydrology();
        computeDerivedFields();
        computeEnvironmentFields();
        generateLakeData();
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
        std::cout << "Hydrologic lake[" << i << "]: level=" << lake.waterLevel
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
    heightSamples.assign(static_cast<std::size_t>(n) * n, settings.baseHeight);
    const unsigned int workerCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int worker = 0; worker < workerCount; ++worker)
    {
        const unsigned int firstRow = n * worker / workerCount;
        const unsigned int lastRow = n * (worker + 1u) / workerCount;
        workers.emplace_back([this, n, spacing, firstRow, lastRow]()
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

                    heightSamples[static_cast<std::size_t>(z) * n + x] =
                        settings.baseHeight + height01 * settings.mountainHeight;
                }
            }
        });
    }
    for (std::thread& worker : workers)
        worker.join();
}

void TerrainMesh::sculptGlacialLandforms()
{
    struct ValleySegment
    {
        glm::vec2 a;
        glm::vec2 b;
        float floorA;
        float floorB;
        float halfWidth;
        float wallRise;
    };
    const std::array<ValleySegment, 4> valleys{{
        {glm::vec2(0.0f, 0.0f), glm::vec2(-2200.0f, 1300.0f),
         1080.0f, 86.0f, 430.0f, 620.0f},
        {glm::vec2(-2200.0f, 1300.0f), glm::vec2(-4100.0f, 2050.0f),
         86.0f, settings.baseHeight + 4.0f, 520.0f, 420.0f},
        {glm::vec2(1620.0f, -760.0f), glm::vec2(-360.0f, 280.0f),
         1260.0f, 760.0f, 250.0f, 430.0f},
        {glm::vec2(900.0f, 1760.0f), glm::vec2(-1450.0f, 960.0f),
         1040.0f, 310.0f, 285.0f, 390.0f}
    }};

    const unsigned int n = settings.resolution;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    for (unsigned int z = 0; z < n; ++z)
    {
        const float worldZ = -settings.size * 0.5f + z * spacing;
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = -settings.size * 0.5f + x * spacing;
            const glm::vec2 p(worldX, worldZ);
            float& height = heightSamples[static_cast<std::size_t>(z) * n + x];
            for (const ValleySegment& valley : valleys)
            {
                const glm::vec2 axis = valley.b - valley.a;
                const float axisLength2 = std::max(glm::dot(axis, axis), 1.0f);
                const float t = glm::clamp(glm::dot(p - valley.a, axis) /
                                           axisLength2, 0.0f, 1.0f);
                const float distance = glm::length(p - (valley.a + axis * t));
                const float normalized = distance / valley.halfWidth;
                if (normalized >= 1.12f)
                    continue;
                const float floor = glm::mix(valley.floorA, valley.floorB, t);
                // A broad floor and rapidly rising walls produce a U-shaped
                // glacial trough instead of a noise-derived V-shaped cut.
                const float wall = std::pow(smoothStep(0.28f, 1.0f, normalized),
                                            1.55f);
                const float target = floor + valley.wallRise * wall;
                const float influence = (1.0f - smoothStep(0.88f, 1.12f,
                                                           normalized)) *
                                        settings.glacialValleyStrength;
                height = std::min(height, glm::mix(height, target, influence));
            }
        }
    }

    const auto carveBasin = [this, n, spacing](const glm::vec2& center,
                                                const glm::vec2& radii,
                                                float depth, float rimHeight,
                                                const glm::vec2& outletDirection)
    {
        const glm::vec2 outlet = glm::normalize(outletDirection);
        for (unsigned int z = 0; z < n; ++z)
        {
            const float worldZ = -settings.size * 0.5f + z * spacing;
            for (unsigned int x = 0; x < n; ++x)
            {
                const float worldX = -settings.size * 0.5f + x * spacing;
                const glm::vec2 delta(worldX - center.x, worldZ - center.y);
                const glm::vec2 q = delta / radii;
                const float r = glm::length(q);
                if (r > 1.18f)
                    continue;
                float& h = heightSamples[static_cast<std::size_t>(z) * n + x];
                if (r < 1.0f)
                {
                    const float bowl = 1.0f - smoothStep(0.0f, 1.0f, r);
                    h -= depth * bowl * bowl;
                }
                const float ring = smoothStep(0.72f, 0.92f, r) *
                                   (1.0f - smoothStep(0.98f, 1.16f, r));
                const glm::vec2 radial = glm::length(delta) > 0.001f
                    ? glm::normalize(delta) : -outlet;
                const float breach = smoothStep(0.72f, 0.96f,
                                                glm::dot(radial, outlet));
                h += rimHeight * ring * (1.0f - breach);
            }
        }
    };
    carveBasin(settings.lakeCenter, settings.lakeRadii,
               settings.lakeBasinDepth * 1.45f, settings.lakeRimHeight,
               settings.meadowLakeCenter - settings.lakeCenter);
    carveBasin(settings.meadowLakeCenter, settings.meadowLakeRadii,
               settings.meadowLakeBasinDepth * 1.35f,
               settings.meadowLakeRimHeight,
               glm::vec2(-1.0f, 0.35f));
}

void TerrainMesh::generateHydrology()
{
    struct FloodNode
    {
        float elevation;
        std::size_t index;
        bool operator>(const FloodNode& other) const
        {
            return elevation > other.elevation;
        }
    };
    struct LakeCandidate
    {
        std::vector<std::size_t> cells;
        float maximumDepth = 0.0f;
        float surface = 0.0f;
    };

    const unsigned int n = settings.resolution;
    const std::size_t count = static_cast<std::size_t>(n) * n;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float cellArea = spacing * spacing;
    const std::array<int, 8> ox{{-1, 0, 1, -1, 1, -1, 0, 1}};
    const std::array<int, 8> oz{{-1, -1, -1, 0, 0, 1, 1, 1}};

    std::vector<float> filled = heightSamples;
    std::vector<int> receiver(count, -1);
    std::vector<unsigned char> visited(count, 0u);
    std::vector<std::size_t> discoveryOrder;
    discoveryOrder.reserve(count);
    std::priority_queue<FloodNode, std::vector<FloodNode>,
                        std::greater<FloodNode>> frontier;
    const auto seed = [&](unsigned int x, unsigned int z)
    {
        const std::size_t i = static_cast<std::size_t>(z) * n + x;
        if (visited[i])
            return;
        visited[i] = 1u;
        frontier.push({filled[i], i});
        discoveryOrder.push_back(i);
    };
    for (unsigned int x = 0; x < n; ++x)
    {
        seed(x, 0u);
        seed(x, n - 1u);
    }
    for (unsigned int z = 1; z + 1u < n; ++z)
    {
        seed(0u, z);
        seed(n - 1u, z);
    }

    while (!frontier.empty())
    {
        const FloodNode current = frontier.top();
        frontier.pop();
        const int cx = static_cast<int>(current.index % n);
        const int cz = static_cast<int>(current.index / n);
        for (std::size_t direction = 0; direction < ox.size(); ++direction)
        {
            const int nx = cx + ox[direction];
            const int nz = cz + oz[direction];
            if (nx < 0 || nz < 0 || nx >= static_cast<int>(n) ||
                nz >= static_cast<int>(n))
                continue;
            const std::size_t next = static_cast<std::size_t>(nz) * n +
                                     static_cast<unsigned int>(nx);
            if (visited[next])
                continue;
            visited[next] = 1u;
            receiver[next] = static_cast<int>(current.index);
            filled[next] = std::max(heightSamples[next], current.elevation);
            frontier.push({filled[next], next});
            discoveryOrder.push_back(next);
        }
    }

    flowAccumulation.resize(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const float normalizedHeight = glm::clamp(
            (heightSamples[i] - settings.baseHeight) / settings.mountainHeight,
            0.0f, 1.0f);
        // Orographic precipitation proxy: high catchments receive more snow
        // and rain, while deterministic detail remains subordinate.
        flowAccumulation[i] = cellArea * glm::mix(0.78f, 1.32f,
                                                  normalizedHeight);
    }
    for (auto it = discoveryOrder.rbegin(); it != discoveryOrder.rend(); ++it)
    {
        const int downstream = receiver[*it];
        if (downstream >= 0)
            flowAccumulation[static_cast<std::size_t>(downstream)] +=
                flowAccumulation[*it];
    }

    std::vector<unsigned char> depression(count, 0u);
    for (std::size_t i = 0; i < count; ++i)
        depression[i] = filled[i] - heightSamples[i] > 0.05f ? 1u : 0u;
    std::vector<unsigned char> classified(count, 0u);
    std::vector<LakeCandidate> candidates;
    std::vector<std::size_t> queue;
    for (std::size_t start = 0; start < count; ++start)
    {
        if (!depression[start] || classified[start])
            continue;
        LakeCandidate candidate;
        queue.clear();
        queue.push_back(start);
        classified[start] = 1u;
        for (std::size_t head = 0; head < queue.size(); ++head)
        {
            const std::size_t i = queue[head];
            candidate.cells.push_back(i);
            candidate.maximumDepth = std::max(candidate.maximumDepth,
                                               filled[i] - heightSamples[i]);
            candidate.surface = std::max(candidate.surface, filled[i]);
            const int x = static_cast<int>(i % n);
            const int z = static_cast<int>(i / n);
            for (std::size_t d = 0; d < ox.size(); ++d)
            {
                const int nx = x + ox[d];
                const int nz = z + oz[d];
                if (nx < 0 || nz < 0 || nx >= static_cast<int>(n) ||
                    nz >= static_cast<int>(n))
                    continue;
                const std::size_t next = static_cast<std::size_t>(nz) * n +
                                         static_cast<unsigned int>(nx);
                if (depression[next] && !classified[next])
                {
                    classified[next] = 1u;
                    queue.push_back(next);
                }
            }
        }
        if (candidate.cells.size() * cellArea >= settings.minimumLakeArea &&
            candidate.maximumDepth >= settings.minimumLakeDepth)
            candidates.push_back(std::move(candidate));
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const LakeCandidate& a, const LakeCandidate& b)
              {
                  return a.cells.size() > b.cells.size();
              });
    if (candidates.size() > MaximumWaterRegions)
        candidates.resize(MaximumWaterRegions);

    hydrologyWaterDepth.assign(count, 0.0f);
    hydrologyWaterSurface.assign(count, 0.0f);
    std::vector<unsigned char> lakeMask(count, 0u);
    lakeRegions.clear();
    for (const LakeCandidate& candidate : candidates)
    {
        glm::vec2 boundsMin(std::numeric_limits<float>::max());
        glm::vec2 boundsMax(-std::numeric_limits<float>::max());
        float maximumDepth = 0.0f;
        std::size_t wetCells = 0u;
        for (const std::size_t i : candidate.cells)
        {
            constexpr float MaximumAlpineLakeDepth = 92.0f;
            float depth = candidate.surface - heightSamples[i];
            if (depth <= 0.05f)
                continue;
            if (depth > MaximumAlpineLakeDepth)
            {
                heightSamples[i] = candidate.surface - MaximumAlpineLakeDepth;
                depth = MaximumAlpineLakeDepth;
            }
            lakeMask[i] = 1u;
            hydrologyWaterDepth[i] = depth;
            hydrologyWaterSurface[i] = candidate.surface;
            maximumDepth = std::max(maximumDepth, depth);
            const unsigned int x = static_cast<unsigned int>(i % n);
            const unsigned int z = static_cast<unsigned int>(i / n);
            const glm::vec2 world(-settings.size * 0.5f + x * spacing,
                                  -settings.size * 0.5f + z * spacing);
            boundsMin = glm::min(boundsMin, world);
            boundsMax = glm::max(boundsMax, world);
            ++wetCells;
        }
        if (wetCells == 0u)
            continue;
        LakeRegion region;
        region.boundsXZ = glm::vec4(boundsMin.x - spacing * 2.0f,
                                    boundsMin.y - spacing * 2.0f,
                                    boundsMax.x + spacing * 2.0f,
                                    boundsMax.y + spacing * 2.0f);
        region.waterLevel = candidate.surface;
        region.area = wetCells * cellArea;
        region.maximumDepth = maximumDepth;
        lakeRegions.push_back(region);
    }

    // Carve every sufficiently large D8 catchment into the conditioned
    // surface. Width and depth scale with sqrt(upstream area), which keeps
    // headwaters narrow and grows trunk rivers smoothly toward the outlet.
    for (std::size_t i = 0; i < count; ++i)
    {
        if (flowAccumulation[i] < settings.riverStartArea || lakeMask[i])
            continue;
        const float sqrtKm2 = std::sqrt(flowAccumulation[i] / 1000000.0f);
        const float width = glm::clamp(settings.riverBaseWidth +
                                       settings.riverWidthPerSqrtKm2 * sqrtKm2,
                                       settings.riverBaseWidth,
                                       settings.riverMaximumWidth);
        const float depth = settings.riverBaseDepth +
                            settings.riverDepthPerSqrtKm2 * sqrtKm2;
        const float channelRadius = std::max(width * 0.5f, spacing * 0.72f);
        // The dry bank is wider than the wet channel.  The previous profile
        // retained 18% of the incision at the last wet sample and then jumped
        // straight back to the untouched terrain one sample later.  A separate
        // bank radius lets both bed depth and terrain height meet continuously.
        const float bankRadius = std::max(channelRadius * 2.35f,
                                          spacing * 1.65f);
        const int gridRadius = std::max(1, static_cast<int>(std::ceil(bankRadius /
                                                                     spacing)));
        const int cx = static_cast<int>(i % n);
        const int cz = static_cast<int>(i / n);
        const float surface = std::max(filled[i] - depth * 0.16f,
                                       settings.baseHeight + 0.08f);
        for (int dz = -gridRadius; dz <= gridRadius; ++dz)
        {
            for (int dx = -gridRadius; dx <= gridRadius; ++dx)
            {
                const int x = cx + dx;
                const int z = cz + dz;
                if (x <= 0 || z <= 0 || x + 1 >= static_cast<int>(n) ||
                    z + 1 >= static_cast<int>(n))
                    continue;
                const float distance = std::sqrt(static_cast<float>(dx * dx +
                                                                     dz * dz)) *
                                       spacing;
                if (distance > bankRadius)
                    continue;
                const std::size_t j = static_cast<std::size_t>(z) * n +
                                      static_cast<unsigned int>(x);
                if (lakeMask[j])
                    continue;
                const float originalHeight = heightSamples[j];
                float targetHeight = originalHeight;
                if (distance <= channelRadius)
                {
                    const float crossSection = 1.0f - smoothStep(
                        0.0f, channelRadius, distance);
                    const float localDepth = depth * std::pow(
                        std::max(crossSection, 0.0f), 0.72f);
                    targetHeight = surface - localDepth;
                }
                else
                {
                    // Dry flood-bank: blend from the water surface back to the
                    // original terrain without extending the water mask.
                    const float bankWeight = 1.0f - smoothStep(
                        channelRadius, bankRadius, distance);
                    targetHeight = glm::mix(originalHeight, surface,
                                            bankWeight);
                }
                heightSamples[j] = std::min(originalHeight, targetHeight);
                const float actualDepth = surface - heightSamples[j];
                if (distance <= channelRadius &&
                    actualDepth > hydrologyWaterDepth[j])
                {
                    hydrologyWaterDepth[j] = actualDepth;
                    hydrologyWaterSurface[j] = surface;
                }
            }
        }
    }

    waterLevel = lakeRegions.empty() ? settings.baseHeight :
                 lakeRegions.front().waterLevel;
    lakeArea = 0.0f;
    maximumWaterDepth = 0.0f;
    glm::vec2 allMin(std::numeric_limits<float>::max());
    glm::vec2 allMax(-std::numeric_limits<float>::max());
    for (std::size_t i = 0; i < count; ++i)
    {
        if (hydrologyWaterSurface[i] > 0.0f)
            hydrologyWaterDepth[i] = std::max(
                hydrologyWaterSurface[i] - heightSamples[i], 0.0f);
        if (hydrologyWaterDepth[i] <= 0.05f)
            continue;
        lakeArea += cellArea;
        maximumWaterDepth = std::max(maximumWaterDepth,
                                     hydrologyWaterDepth[i]);
        const unsigned int x = static_cast<unsigned int>(i % n);
        const unsigned int z = static_cast<unsigned int>(i / n);
        const glm::vec2 world(-settings.size * 0.5f + x * spacing,
                              -settings.size * 0.5f + z * spacing);
        allMin = glm::min(allMin, world);
        allMax = glm::max(allMax, world);
    }
    lakeBoundsXZ = lakeArea > 0.0f
        ? glm::vec4(allMin.x, allMin.y, allMax.x, allMax.y)
        : glm::vec4(0.0f);
}

void TerrainMesh::generateLakeData()
{
    const unsigned int lakeResolution = settings.lakeDataResolution;
    const std::size_t lakeSampleCount =
        static_cast<std::size_t>(lakeResolution) * lakeResolution;
    const float lakeSpacing = settings.size /
                              static_cast<float>(lakeResolution - 1u);
    const unsigned int sourceResolution = settings.resolution;
    std::vector<float> depth(lakeSampleCount, 0.0f);
    std::vector<float> surface(lakeSampleCount, 0.0f);
    std::vector<unsigned char> wet(lakeSampleCount, 0u);
    std::vector<float> sourceWet(hydrologyWaterDepth.size(), 0.0f);
    std::vector<float> weightedSurface(hydrologyWaterDepth.size(), 0.0f);
    for (std::size_t i = 0; i < hydrologyWaterDepth.size(); ++i)
    {
        sourceWet[i] = hydrologyWaterDepth[i] > 0.04f ? 1.0f : 0.0f;
        weightedSurface[i] = hydrologyWaterSurface[i] * sourceWet[i];
    }
    const auto bilinear = [sourceResolution](const std::vector<float>& field,
                                             float gx, float gz)
    {
        const unsigned int x0 = std::min(static_cast<unsigned int>(gx),
                                         sourceResolution - 2u);
        const unsigned int z0 = std::min(static_cast<unsigned int>(gz),
                                         sourceResolution - 2u);
        const float tx = gx - static_cast<float>(x0);
        const float tz = gz - static_cast<float>(z0);
        const float a = glm::mix(field[static_cast<std::size_t>(z0) *
                                       sourceResolution + x0],
                                 field[static_cast<std::size_t>(z0) *
                                       sourceResolution + x0 + 1u], tx);
        const float b = glm::mix(field[static_cast<std::size_t>(z0 + 1u) *
                                       sourceResolution + x0],
                                 field[static_cast<std::size_t>(z0 + 1u) *
                                       sourceResolution + x0 + 1u], tx);
        return glm::mix(a, b, tz);
    };
    for (unsigned int z = 0; z < lakeResolution; ++z)
    {
        const float gz = static_cast<float>(z) *
                         static_cast<float>(sourceResolution - 1u) /
                         static_cast<float>(lakeResolution - 1u);
        for (unsigned int x = 0; x < lakeResolution; ++x)
        {
            const float gx = static_cast<float>(x) *
                             static_cast<float>(sourceResolution - 1u) /
                             static_cast<float>(lakeResolution - 1u);
            const std::size_t i = static_cast<std::size_t>(z) * lakeResolution + x;
            const float wetWeight = bilinear(sourceWet, gx, gz);
            surface[i] = wetWeight > 0.001f
                ? bilinear(weightedSurface, gx, gz) / wetWeight : 0.0f;
            const float terrainHeight = bilinear(heightSamples, gx, gz);
            // Derive coverage from the continuous water-surface/terrain
            // intersection. Gating this with the interpolated binary mask made
            // deep, narrow rivers lose depth at their outer LDM texels and
            // produced a visible/cpu-contract discontinuity at the bank.
            depth[i] = wetWeight > 0.001f
                ? std::max(surface[i] - terrainHeight, 0.0f) : 0.0f;
            wet[i] = depth[i] > 0.04f ? 1u : 0u;
        }
    }

    const float infinity = std::numeric_limits<float>::max() * 0.25f;
    std::vector<float> insideDistance(lakeSampleCount, infinity);
    std::vector<float> outsideDistance(lakeSampleCount, infinity);
    std::vector<float> nearestSurface = surface;
    for (std::size_t i = 0; i < lakeSampleCount; ++i)
    {
        if (wet[i])
            outsideDistance[i] = 0.0f;
        else
        {
            insideDistance[i] = 0.0f;
            nearestSurface[i] = 0.0f;
        }
    }
    const auto relax = [&](std::size_t target, std::size_t sourceIndex,
                           float cost, bool copySurface)
    {
        std::vector<float>& distance = copySurface ? outsideDistance
                                                   : insideDistance;
        const float candidate = distance[sourceIndex] + cost;
        if (candidate < distance[target])
        {
            distance[target] = candidate;
            if (copySurface)
                nearestSurface[target] = nearestSurface[sourceIndex];
        }
    };
    constexpr float Diagonal = 1.41421356f;
    for (unsigned int z = 0; z < lakeResolution; ++z)
    {
        for (unsigned int x = 0; x < lakeResolution; ++x)
        {
            const std::size_t i = static_cast<std::size_t>(z) * lakeResolution + x;
            if (x > 0u)
            {
                relax(i, i - 1u, 1.0f, false);
                relax(i, i - 1u, 1.0f, true);
            }
            if (z > 0u)
            {
                relax(i, i - lakeResolution, 1.0f, false);
                relax(i, i - lakeResolution, 1.0f, true);
                if (x > 0u)
                {
                    relax(i, i - lakeResolution - 1u, Diagonal, false);
                    relax(i, i - lakeResolution - 1u, Diagonal, true);
                }
                if (x + 1u < lakeResolution)
                {
                    relax(i, i - lakeResolution + 1u, Diagonal, false);
                    relax(i, i - lakeResolution + 1u, Diagonal, true);
                }
            }
        }
    }
    for (int z = static_cast<int>(lakeResolution) - 1; z >= 0; --z)
    {
        for (int x = static_cast<int>(lakeResolution) - 1; x >= 0; --x)
        {
            const std::size_t i = static_cast<std::size_t>(z) * lakeResolution +
                                  static_cast<unsigned int>(x);
            if (x + 1 < static_cast<int>(lakeResolution))
            {
                relax(i, i + 1u, 1.0f, false);
                relax(i, i + 1u, 1.0f, true);
            }
            if (z + 1 < static_cast<int>(lakeResolution))
            {
                relax(i, i + lakeResolution, 1.0f, false);
                relax(i, i + lakeResolution, 1.0f, true);
                if (x > 0)
                {
                    relax(i, i + lakeResolution - 1u, Diagonal, false);
                    relax(i, i + lakeResolution - 1u, Diagonal, true);
                }
                if (x + 1 < static_cast<int>(lakeResolution))
                {
                    relax(i, i + lakeResolution + 1u, Diagonal, false);
                    relax(i, i + lakeResolution + 1u, Diagonal, true);
                }
            }
        }
    }

    lakeDataHalf.resize(lakeSampleCount * 3u);
    for (std::size_t i = 0; i < lakeSampleCount; ++i)
    {
        const float signedDistance = wet[i]
            ? insideDistance[i] * lakeSpacing
            : -outsideDistance[i] * lakeSpacing;
        lakeDataHalf[i * 3u] = floatToHalf(depth[i]);
        lakeDataHalf[i * 3u + 1u] = floatToHalf(glm::clamp(
            signedDistance, -128.0f, 128.0f));
        lakeDataHalf[i * 3u + 2u] = floatToHalf(
            wet[i] ? surface[i] : nearestSurface[i]);
    }
    flowAccumulation.clear();
    flowAccumulation.shrink_to_fit();
    hydrologyWaterDepth.clear();
    hydrologyWaterDepth.shrink_to_fit();
    hydrologyWaterSurface.clear();
    hydrologyWaterSurface.shrink_to_fit();
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

void TerrainMesh::computeEnvironmentFields()
{
    const unsigned int n = settings.resolution;
    const std::size_t count = static_cast<std::size_t>(n) * n;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    std::vector<float> waterInfluence(count, 0.0f);
    for (std::size_t i = 0; i < count; ++i)
        waterInfluence[i] = hydrologyWaterDepth[i] > 0.04f ? 1.0f : 0.0f;
    const float decay = std::exp(-spacing / 135.0f);
    // Four directional sweeps approximate distance-to-water at terrain-grid
    // scale without introducing an unrelated noise field.
    for (int pass = 0; pass < 3; ++pass)
    {
        for (unsigned int z = 0; z < n; ++z)
            for (unsigned int x = 0; x < n; ++x)
            {
                const std::size_t i = static_cast<std::size_t>(z) * n + x;
                if (x > 0u)
                    waterInfluence[i] = std::max(waterInfluence[i],
                                                 waterInfluence[i - 1u] * decay);
                if (z > 0u)
                    waterInfluence[i] = std::max(waterInfluence[i],
                        waterInfluence[i - n] * decay);
            }
        for (int z = static_cast<int>(n) - 1; z >= 0; --z)
            for (int x = static_cast<int>(n) - 1; x >= 0; --x)
            {
                const std::size_t i = static_cast<std::size_t>(z) * n +
                                      static_cast<unsigned int>(x);
                if (x + 1 < static_cast<int>(n))
                    waterInfluence[i] = std::max(waterInfluence[i],
                                                 waterInfluence[i + 1u] * decay);
                if (z + 1 < static_cast<int>(n))
                    waterInfluence[i] = std::max(waterInfluence[i],
                        waterInfluence[i + n] * decay);
            }
    }

    terrainEnvironmentHalf.resize(count * 2u);
    for (unsigned int z = 0; z < n; ++z)
    {
        const unsigned int zd = z > 0u ? z - 1u : z;
        const unsigned int zu = z + 1u < n ? z + 1u : z;
        for (unsigned int x = 0; x < n; ++x)
        {
            const unsigned int xl = x > 0u ? x - 1u : x;
            const unsigned int xr = x + 1u < n ? x + 1u : x;
            const std::size_t i = static_cast<std::size_t>(z) * n + x;
            const float h = halfToFloat(terrainDataHalf[i * 4u]);
            const float slope = halfToFloat(terrainDataHalf[i * 4u + 1u]);
            const float curvature = halfToFloat(terrainDataHalf[i * 4u + 3u]);
            const float dhdz = (heightSamples[static_cast<std::size_t>(zu) * n + x] -
                                heightSamples[static_cast<std::size_t>(zd) * n + x]) /
                               std::max(static_cast<float>(zu - zd) * spacing, 0.001f);
            const float dhdx = (heightSamples[static_cast<std::size_t>(z) * n + xr] -
                                heightSamples[static_cast<std::size_t>(z) * n + xl]) /
                               std::max(static_cast<float>(xr - xl) * spacing, 0.001f);
            const glm::vec3 normal = glm::normalize(glm::vec3(-dhdx, 1.0f, -dhdz));
            const float northness = normal.z * 0.5f + 0.5f;
            const float catchment = glm::clamp(
                std::log1p(flowAccumulation[i] / 5000.0f) /
                std::log1p(18000000.0f / 5000.0f), 0.0f, 1.0f);
            const float concavity = smoothStep(0.50f, 0.86f, curvature);
            const float ridgeDrying = smoothStep(0.18f, 0.52f,
                                                  1.0f - curvature);
            float moisture = 0.08f + catchment * 0.43f +
                             waterInfluence[i] * 0.42f + concavity * 0.17f +
                             northness * 0.10f - slope * 0.18f -
                             ridgeDrying * 0.08f;
            moisture = glm::clamp(moisture, 0.0f, 1.0f);

            const float altitudeSnow = smoothStep(0.50f, 0.72f, h);
            const float stableSlope = 1.0f - smoothStep(0.42f, 0.78f, slope);
            const float leeDeposit = glm::mix(0.58f, 1.0f, northness);
            const float hollowRetention = glm::mix(0.78f, 1.12f, concavity);
            const float retainedSnow = glm::clamp(
                altitudeSnow * stableSlope * leeDeposit * hollowRetention *
                glm::mix(0.82f, 1.0f, moisture), 0.0f, 1.0f);
            // Above the permanent snow line the PBR snow cover is complete;
            // slope/aspect only control the lower, seasonal transition band.
            const float permanentSnow = smoothStep(0.70f, 0.80f, h);
            const float snow = std::max(retainedSnow, permanentSnow);
            terrainEnvironmentHalf[i * 2u] = floatToHalf(moisture);
            terrainEnvironmentHalf[i * 2u + 1u] = floatToHalf(snow);
        }
    }
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

    glGenTextures(1, &terrainEnvironmentTexture);
    glBindTexture(GL_TEXTURE_2D, terrainEnvironmentTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, n, n, 0, GL_RG,
                 GL_HALF_FLOAT, terrainEnvironmentHalf.data());
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
    textures.push_back({terrainEnvironmentTexture, "texture_terrainEnvironment",
                        "terrain://environment"});
    textures.push_back({lakeDataTexture, "texture_terrainLakeData",
                        "terrain://shore-distance"});
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

glm::vec2 TerrainMesh::sampleEnvironmentData(float worldX, float worldZ) const
{
    const unsigned int n = settings.resolution;
    if (terrainEnvironmentHalf.empty() || n < 2u)
        return glm::vec2(0.0f);
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f || gridX > n - 1.0f || gridZ > n - 1.0f)
        return glm::vec2(0.0f);
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const auto read = [this, n](unsigned int x, unsigned int z)
    {
        const std::size_t i = (static_cast<std::size_t>(z) * n + x) * 2u;
        return glm::vec2(halfToFloat(terrainEnvironmentHalf[i]),
                         halfToFloat(terrainEnvironmentHalf[i + 1u]));
    };
    const glm::vec2 a = glm::mix(read(x0, z0), read(x0 + 1u, z0), tx);
    const glm::vec2 b = glm::mix(read(x0, z0 + 1u), read(x0 + 1u, z0 + 1u), tx);
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
    const glm::vec2 environment = sampleEnvironmentData(worldX, worldZ);
    sample.moisture = environment.x;
    sample.snowRetention = environment.y;
    const glm::vec2 lake = sampleLakeData(worldX, worldZ);
    sample.signedDistanceToWater = lake.y;
    sample.waterSurfaceHeight = sampleWaterSurfaceHeight(worldX, worldZ);
    sample.waterDepth = lake.y >= 0.0f
        ? std::max(sample.waterSurfaceHeight - renderedHeight, 0.0f) : 0.0f;
    sample.underwater = sample.waterDepth > 0.01f;
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
    const glm::vec2 lake = sampleLakeData(x, z);
    if (lake.y < 0.0f)
        return 0.0f;
    return std::max(sampleWaterSurfaceHeight(x, z) - sampleHeight(x, z),
                    0.0f);
}

float TerrainMesh::sampleSignedDistanceToWater(float x, float z) const
{
    return sampleLakeData(x, z).y;
}

float TerrainMesh::sampleWaterSurfaceHeight(float worldX, float worldZ) const
{
    const unsigned int n = settings.lakeDataResolution;
    if (lakeDataHalf.empty() || n < 2u)
        return 0.0f;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    const float gridX = (worldX + settings.size * 0.5f) / spacing;
    const float gridZ = (worldZ + settings.size * 0.5f) / spacing;
    if (gridX < 0.0f || gridZ < 0.0f || gridX > n - 1.0f || gridZ > n - 1.0f)
        return 0.0f;
    const unsigned int x0 = std::min(static_cast<unsigned int>(gridX), n - 2u);
    const unsigned int z0 = std::min(static_cast<unsigned int>(gridZ), n - 2u);
    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const auto read = [this, n](unsigned int x, unsigned int z)
    {
        const std::size_t i = (static_cast<std::size_t>(z) * n + x) * 3u + 2u;
        return halfToFloat(lakeDataHalf[i]);
    };
    const float a = glm::mix(read(x0, z0), read(x0 + 1u, z0), tx);
    const float b = glm::mix(read(x0, z0 + 1u), read(x0 + 1u, z0 + 1u), tx);
    return glm::mix(a, b, tz);
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
