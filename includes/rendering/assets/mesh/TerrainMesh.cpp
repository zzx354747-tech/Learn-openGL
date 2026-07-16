#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace
{
constexpr std::uint32_t CacheMagic = 0x4D445441u; // ATDM
constexpr std::uint32_t CacheVersion = 4u;
constexpr float Pi = 3.14159265358979323846f;

struct CacheHeader
{
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t resolution;
    std::uint32_t reserved;
    std::uint64_t parameterHash;
    float waterLevel;
    float padding[3];
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
    if (detailNoiseTexture)
        glDeleteTextures(1, &detailNoiseTexture);
}

std::uint64_t TerrainMesh::parameterHash() const
{
    std::uint64_t hash = 1469598103934665603ull;
    hashBytes(hash, &CacheVersion, sizeof(CacheVersion));
    hashBytes(hash, &settings.resolution, sizeof(settings.resolution));
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
    hashBytes(hash, &settings.lakeBasinDepth, sizeof(settings.lakeBasinDepth));
    hashBytes(hash, &settings.lakeLevelOffset, sizeof(settings.lakeLevelOffset));
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
        header.parameterHash != parameterHash())
    {
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(settings.resolution) *
                              settings.resolution;
    heightSamples.resize(count);
    terrainDataHalf.resize(count * 4u);
    if (!stream.read(reinterpret_cast<char*>(heightSamples.data()),
                     static_cast<std::streamsize>(count * sizeof(float))) ||
        !stream.read(reinterpret_cast<char*>(terrainDataHalf.data()),
                     static_cast<std::streamsize>(count * 4u * sizeof(std::uint16_t))))
    {
        heightSamples.clear();
        terrainDataHalf.clear();
        return false;
    }
    waterLevel = header.waterLevel;
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
    const CacheHeader header{CacheMagic, CacheVersion, settings.resolution, 0u,
                             parameterHash(), waterLevel, {0.0f, 0.0f, 0.0f}};
    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    stream.write(reinterpret_cast<const char*>(heightSamples.data()),
                 static_cast<std::streamsize>(heightSamples.size() * sizeof(float)));
    stream.write(reinterpret_cast<const char*>(terrainDataHalf.data()),
                 static_cast<std::streamsize>(terrainDataHalf.size() * sizeof(std::uint16_t)));
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
        findWaterLevel();
        computeDerivedFields();
        saveCache();
    }
    uploadTerrainTextures();
    buildMesh();
    const double milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "Terrain: " << (cacheHit ? "cache hit" : "generated")
              << ", ready in " << milliseconds << " ms" << std::endl;
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

                    // Carve one true local basin. Only terrain height defines
                    // its eventual shoreline.
                    const glm::vec2 lakeDelta =
                        (glm::vec2(worldX, worldZ) - settings.lakeCenter) /
                        settings.lakeRadii;
                    const float lakeR2 = glm::dot(lakeDelta, lakeDelta);
                    const float basin = std::exp(-lakeR2 * 2.3f) *
                                        settings.lakeBasinDepth;
                    heightSamples[static_cast<std::size_t>(z) * n + x] =
                        settings.baseHeight + height01 * settings.mountainHeight - basin;
                }
            }
        });
    }
    for (std::thread& worker : workers)
        worker.join();
}

void TerrainMesh::findWaterLevel()
{
    const unsigned int n = settings.resolution;
    const float spacing = settings.size / static_cast<float>(n - 1u);
    float minimum = std::numeric_limits<float>::max();
    for (unsigned int z = 0; z < n; ++z)
    {
        const float worldZ = -settings.size * 0.5f + z * spacing;
        for (unsigned int x = 0; x < n; ++x)
        {
            const float worldX = -settings.size * 0.5f + x * spacing;
            const glm::vec2 d = (glm::vec2(worldX, worldZ) - settings.lakeCenter) /
                                settings.lakeRadii;
            if (glm::dot(d, d) <= 0.35f)
                minimum = std::min(minimum, heightSamples[static_cast<std::size_t>(z) * n + x]);
        }
    }
    waterLevel = minimum + settings.lakeLevelOffset;
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
bool TerrainMesh::isBelowWater(float x, float z) const { return sampleHeight(x, z) < waterLevel; }

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
