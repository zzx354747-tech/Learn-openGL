#include "rendering/assets/mesh/AlpineVegetationSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <unordered_map>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/assets/mesh/AlpineBiome.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/uniforms/TemporalJitter.h"
#include "scene/Camera.h"

namespace
{
constexpr std::uint32_t CacheMagic = 0x56475041u; // APGV
constexpr std::uint32_t CacheVersion = 11u;
constexpr int DensityResolution = 256;
constexpr float Pi = 3.14159265358979323846f;

struct CacheHeader
{
    std::uint32_t magic;
    std::uint32_t version;
    std::uint64_t terrainHash;
    std::uint64_t settingsHash;
    std::uint32_t speciesCount;
    std::uint32_t reserved;
};

std::filesystem::path cacheDirectory()
{
#ifdef OPENGL_PROJECT_ROOT
    return std::filesystem::path(OPENGL_PROJECT_ROOT) / "build" / "vegetation_cache";
#else
    return std::filesystem::path("../build/vegetation_cache");
#endif
}

std::uint32_t hash32(std::uint32_t value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    return value ^ (value >> 16u);
}

float random01(std::uint32_t value)
{
    return static_cast<float>(hash32(value) & 0x00ffffffu) / 16777215.0f;
}

float fade(float t) { return t * t * (3.0f - 2.0f * t); }

float valueNoise(glm::vec2 p, std::uint32_t seed)
{
    const glm::ivec2 cell = glm::ivec2(glm::floor(p));
    const glm::vec2 f = glm::fract(p);
    const auto h = [seed](int x, int y)
    {
        return random01(static_cast<std::uint32_t>(x) * 0x8da6b343u ^
                        static_cast<std::uint32_t>(y) * 0xd8163841u ^ seed);
    };
    const float a = h(cell.x, cell.y);
    const float b = h(cell.x + 1, cell.y);
    const float c = h(cell.x, cell.y + 1);
    const float d = h(cell.x + 1, cell.y + 1);
    const glm::vec2 u(fade(f.x), fade(f.y));
    return glm::mix(glm::mix(a, b, u.x), glm::mix(c, d, u.x), u.y);
}

float fractf(float x) { return x - std::floor(x); }

float elapsedSeconds()
{
    using Clock = std::chrono::steady_clock;
    static const auto start = Clock::now();
    return std::chrono::duration<float>(Clock::now() - start).count();
}

std::uint32_t packVariation(float hue, float value, std::uint8_t shape,
                            std::uint8_t flags)
{
    const auto byte = [](float v)
    {
        return static_cast<std::uint32_t>(glm::clamp(std::round(v * 255.0f), 0.0f, 255.0f));
    };
    // IEEE float represents every 24-bit integer exactly. The high shape bit
    // carries the dry-grass flag without relying on NaN bit-pattern payloads.
    const std::uint8_t packedShape = static_cast<std::uint8_t>(
        (shape & 0x7fu) | (flags ? 0x80u : 0u));
    return byte(hue) | (byte(value) << 8u) |
           (static_cast<std::uint32_t>(packedShape) << 16u);
}

AlpineBiomeParameters biomeParameters(const SceneRenderConfig& config)
{
    AlpineBiomeParameters p;
    p.sunAzimuth = config.terrainSunAzimuth;
    p.sunHeightShift = config.terrainSunHeightShift;
    p.noiseHeightShift = config.terrainNoiseHeightShift;
    p.grassEnd = config.terrainGrassEnd;
    p.rockStart = config.terrainRockStart;
    p.snowStart = config.terrainSnowStart;
    p.snowEnd = config.terrainSnowEnd;
    return p;
}

bool isTree(std::size_t i) { return i <= 2u; }
bool isShrub(std::size_t i) { return i >= 3u && i <= 4u; }
bool isGrass(std::size_t i) { return i >= 5u && i <= 7u; }
bool isFlower(std::size_t i) { return i >= 8u && i <= 10u; }
bool isCushion(std::size_t i) { return i >= 11u; }
}

AlpineVegetationSystem::AlpineVegetationSystem(const TerrainMesh& terrain)
    : AlpineVegetationSystem(terrain, Settings{})
{}

AlpineVegetationSystem::AlpineVegetationSystem(const TerrainMesh& terrain,
                                               const Settings& settings)
    : terrain_(terrain), settings_(settings)
{
    const auto startupBegin = std::chrono::steady_clock::now();
    settings_.chunkSize = std::max(settings_.chunkSize, 64.0f);
    buildMeshSets();
    validateBiomeParity();
    cacheHit_ = loadCache();
    if (!cacheHit_)
    {
        generateDistribution();
        saveCache();
    }
    else
    {
        for (Bucket& bucket : buckets_)
            buildChunks(bucket);
    }
    buildDensityTexture();
    upload();
    const float startupMs = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - startupBegin).count();
    std::cout << "Alpine vegetation: " << instanceCount() << " instances ("
              << (cacheHit_ ? "cache" : "generated") << "), ready in "
              << startupMs << " ms" << std::endl;
    std::cout << "  species: trees=" << buckets_[0].instances.size() << "/"
              << buckets_[1].instances.size() << "/" << buckets_[2].instances.size()
              << ", shrubs=" << buckets_[3].instances.size() << "/"
              << buckets_[4].instances.size() << ", grass="
              << buckets_[5].instances.size() << "/" << buckets_[6].instances.size()
              << "/" << buckets_[7].instances.size() << ", flowers="
              << buckets_[8].instances.size() << "/" << buckets_[9].instances.size()
              << "/" << buckets_[10].instances.size() << ", cushion="
              << buckets_[11].instances.size() << "/" << buckets_[12].instances.size()
              << std::endl;
}

AlpineVegetationSystem::~AlpineVegetationSystem()
{
    destroyGpuResources();
}

void AlpineVegetationSystem::buildMeshSets()
{
    const std::uint32_t s = settings_.seed;
    const std::array<Species, static_cast<std::size_t>(Species::Count)> species = {{
        Species::ConiferTall, Species::ConiferBroad, Species::ConiferSapling,
        Species::ShrubRound, Species::ShrubWindSwept,
        Species::GrassA, Species::GrassB, Species::GrassC,
        Species::FlowerStar, Species::FlowerBell, Species::FlowerSpike,
        Species::CushionA, Species::CushionB}};
    for (std::size_t i = 0; i < buckets_.size(); ++i)
        buckets_[i].species = species[i];
    buckets_[0].cpuMeshes = AlpineVegetationMeshFactory::makeTallConifer(s + 11u);
    buckets_[1].cpuMeshes = AlpineVegetationMeshFactory::makeBroadConifer(s + 23u);
    buckets_[2].cpuMeshes = AlpineVegetationMeshFactory::makeSapling(s + 37u);
    buckets_[3].cpuMeshes = AlpineVegetationMeshFactory::makeRoundShrub(s + 41u);
    buckets_[4].cpuMeshes = AlpineVegetationMeshFactory::makeWindSweptShrub(s + 53u);
    buckets_[5].cpuMeshes = AlpineVegetationMeshFactory::makeGrassTuftA(s + 67u);
    buckets_[6].cpuMeshes = AlpineVegetationMeshFactory::makeGrassTuftB(s + 79u);
    buckets_[7].cpuMeshes = AlpineVegetationMeshFactory::makeGrassTuftC(s + 83u);
    buckets_[8].cpuMeshes = AlpineVegetationMeshFactory::makeStarFlower(s + 97u);
    buckets_[9].cpuMeshes = AlpineVegetationMeshFactory::makeBellFlower(s + 101u);
    buckets_[10].cpuMeshes = AlpineVegetationMeshFactory::makeSpikeFlower(s + 107u);
    buckets_[11].cpuMeshes = AlpineVegetationMeshFactory::makeCushionPlantA(s + 109u);
    buckets_[12].cpuMeshes = AlpineVegetationMeshFactory::makeCushionPlantB(s + 127u);
}

float AlpineVegetationSystem::sampleDensityField(const glm::vec2& worldXZ) const
{
    // Macro distribution comes from the same catchment/slope/curvature field
    // used by terrain materials. Noise only breaks up local plant spacing.
    const float moisture = terrain_.sampleEnvironmentData(worldXZ.x,
                                                           worldXZ.y).x;
    const float detail = valueNoise(worldXZ / 150.0f, settings_.seed + 877u);
    return glm::clamp(moisture * glm::mix(0.86f, 1.12f, detail), 0.0f, 1.0f);
}

void AlpineVegetationSystem::generateDistribution()
{
    const auto start = std::chrono::steady_clock::now();
    const float size = terrain_.getSettings().size;
    const float half = size * 0.48f;
    SceneRenderConfig defaults;
    const AlpineBiomeParameters bp = biomeParameters(defaults);
    std::vector<float> treeDensity(static_cast<std::size_t>(DensityResolution) * DensityResolution, 0.0f);

    struct HabitatPatch
    {
        glm::vec2 center;
        float radius;
    };
    std::vector<HabitatPatch> meadowPatches;
    std::vector<HabitatPatch> flowerPatches;
    const auto suitableGrass = [&](const glm::vec2& xz, float slopeLimit)
    {
        const TerrainMesh::SurfaceSample surface = terrain_.sampleSurface(xz.x, xz.y);
        const float noise = valueNoise(xz / 480.0f, settings_.seed + 1301u);
        const glm::vec3 biome = alpineBiomeWeights(surface.tdm.x, surface.tdm.z,
                                                    noise, bp);
        return !surface.underwater && surface.tdm.y < slopeLimit && biome.x > 0.72f;
    };

    // Author a visible meadow system around the designed plateau lake, then
    // add deterministic biome-valid clearings across the rest of the terrain.
    // Instances remain fully procedural; these are habitat domains, not hand
    // placed individual plants.
    const TerrainMesh::Settings& terrainSettings = terrain_.getSettings();
    // Overlapping domains around the meadow lake provide a continuous base
    // meadow in the intended starting area. Terrain/biome tests still reject
    // water, rock and steep faces point-by-point.
    for (int gz = -2; gz <= 2; ++gz)
    {
        for (int gx = -2; gx <= 2; ++gx)
        {
            const glm::vec2 center = terrainSettings.meadowLakeCenter +
                glm::vec2(static_cast<float>(gx) * 215.0f,
                          static_cast<float>(gz) * 185.0f);
            meadowPatches.push_back({center, 310.0f});
        }
    }
    for (std::uint32_t i = 0; i < 20u; ++i)
    {
        const float angle = 2.0f * Pi * (static_cast<float>(i) / 20.0f) +
                            random01(settings_.seed + i * 97u) * 0.18f;
        const float ring = 1.18f + random01(settings_.seed + i * 193u) * 1.15f;
        const glm::vec2 offset(std::cos(angle) * terrainSettings.meadowLakeRadii.x * ring,
                               std::sin(angle) * terrainSettings.meadowLakeRadii.y * ring);
        const glm::vec2 center = terrainSettings.meadowLakeCenter + offset;
        if (suitableGrass(center, 0.42f))
            meadowPatches.push_back({center, 180.0f + random01(i * 271u + 5u) * 130.0f});
    }
    for (std::uint32_t attempt = 0; attempt < 20000u && meadowPatches.size() < 72u; ++attempt)
    {
        const glm::vec2 center(
            glm::mix(-half, half, fractf((attempt + 0.5f) * 0.61803398875f)),
            glm::mix(-half, half, fractf((attempt + 0.5f) * 0.75487766625f)));
        if (suitableGrass(center, 0.36f) && sampleDensityField(center) > 0.34f)
            meadowPatches.push_back({center, 150.0f + random01(attempt * 313u + 7u) * 170.0f});
    }
    for (std::size_t i = 0; i < meadowPatches.size(); i += 2u)
    {
        const HabitatPatch& meadow = meadowPatches[i];
        flowerPatches.push_back({meadow.center + glm::vec2(
            (random01(static_cast<std::uint32_t>(i) * 401u + 11u) - 0.5f) * meadow.radius * 0.55f,
            (random01(static_cast<std::uint32_t>(i) * 409u + 13u) - 0.5f) * meadow.radius * 0.55f),
            meadow.radius * (0.48f + random01(static_cast<std::uint32_t>(i) * 419u + 17u) * 0.25f)});
    }
    // Give the default meadow viewpoint several compact, overlapping flower
    // fields. Duplicating these domains is intentional weighting: roughly half
    // of the flower budget becomes a readable near-field flower sea while the
    // remaining domains still distribute colour across the wider grassland.
    const glm::vec2 meadowView = terrainSettings.meadowLakeCenter +
        glm::vec2(0.0f, terrainSettings.meadowLakeRadii.y * 2.35f);
    constexpr std::array<glm::vec2, 6> LocalFlowerOffsets = {{
        {-115.0f, -80.0f}, {95.0f, -65.0f}, {-55.0f, 45.0f},
        {125.0f, 80.0f}, {-145.0f, 120.0f}, {20.0f, 155.0f}}};
    for (int repeat = 0; repeat < 6; ++repeat)
        for (std::size_t i = 0; i < LocalFlowerOffsets.size(); ++i)
            flowerPatches.push_back({meadowView + LocalFlowerOffsets[i],
                105.0f + 12.0f * static_cast<float>((i + repeat) % 3u)});
    if (meadowPatches.empty())
        meadowPatches.push_back({terrainSettings.meadowLakeCenter + glm::vec2(0.0f, 500.0f), 180.0f});
    if (flowerPatches.empty())
        flowerPatches.push_back({meadowPatches.front().center, meadowPatches.front().radius * 0.45f});
    std::cout << "Vegetation habitats: " << meadowPatches.size()
              << " meadow patches, " << flowerPatches.size()
              << " flower fields" << std::endl;

    const auto addCategory = [&](std::size_t firstBucket, std::size_t bucketCount,
                                 std::uint32_t budget, float minimumSpacing,
                                 int category)
    {
        const int occupancyResolution = minimumSpacing > 0.0f
            ? static_cast<int>(std::ceil(size / minimumSpacing)) : 0;
        std::unordered_map<std::uint64_t, glm::vec2> occupancy;
        if (minimumSpacing > 0.0f)
            occupancy.reserve(static_cast<std::size_t>(budget) * 2u);
        std::uint32_t accepted = 0;
        const std::uint32_t attemptsPerInstance = category == 4 ? 1600u :
            (category == 2 ? 8u : category == 3 ? 14u : 28u);
        const std::uint32_t maxAttempts = std::max(budget * attemptsPerInstance, 1000u);
        for (std::uint32_t attempt = 0; attempt < maxAttempts && accepted < budget; ++attempt)
        {
            const float u = fractf((attempt + 0.5f) * 0.61803398875f +
                                   random01(settings_.seed + category * 101u));
            const float v = fractf((attempt + 0.5f) * 0.75487766625f +
                                   random01(settings_.seed + category * 313u));
            const float jitterX = (random01(attempt * 17u + category * 977u) - 0.5f) * 1.7f;
            const float jitterZ = (random01(attempt * 29u + category * 1237u) - 0.5f) * 1.7f;
            glm::vec2 xz(glm::mix(-half, half, u) + jitterX,
                         glm::mix(-half, half, v) + jitterZ);
            // Grass uses both a broad biome-wide base layer and overlapping
            // meadow domains. Flowers stay clustered, but their larger,
            // overlapping domains produce readable flower fields rather than
            // isolated speckles. Grass is predominantly biome-wide; flowers
            // keep dense fields but also receive a biome-wide base layer.
            const bool useHabitatPatch =
                (category == 2 && (attempt & 3u) == 0u) ||
                (category == 3 && (attempt & 3u) != 0u);
            if (useHabitatPatch)
            {
                const std::vector<HabitatPatch>& patches = category == 2
                    ? meadowPatches : flowerPatches;
                const HabitatPatch& patch = patches[attempt % patches.size()];
                const float angle = random01(attempt * 811u + category * 43u) * 2.0f * Pi;
                const float edgeNoise = 0.94f + 0.06f * std::sin(angle * 5.0f +
                    static_cast<float>(attempt % patches.size()));
                const float radius = patch.radius * edgeNoise *
                    std::sqrt(random01(attempt * 823u + category * 59u));
                xz = patch.center + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
            }
            const TerrainMesh::SurfaceSample surface = terrain_.sampleSurface(xz.x, xz.y);
            const glm::vec4 tdm = surface.tdm;
            if (surface.underwater)
                continue;
            const float density = sampleDensityField(xz);
            const float macroNoise = valueNoise(xz / 480.0f, settings_.seed + 1301u);
            const glm::vec3 biome = alpineBiomeWeights(tdm.x, tdm.z, macroNoise, bp);
            const float sunFacing = std::cos(tdm.z * 2.0f * Pi - bp.sunAzimuth);
            float probability = 0.0f;
            if (category == 0)
            {
                const float treeLine = 0.45f + sunFacing * 0.05f;
                const float lineFade = 1.0f - alpineSmoothstep(treeLine - 0.035f,
                                                               treeLine + 0.025f, tdm.x);
                probability = biome.x * lineFade * (tdm.y < 0.39f ? 1.0f : 0.0f) *
                              glm::mix(0.18f, 1.0f, density) *
                              (tdm.w > 0.55f ? 1.4f : 1.0f) * 0.64f;
            }
            else if (category == 1)
            {
                const int px = std::clamp(static_cast<int>((xz.x / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                const int py = std::clamp(static_cast<int>((xz.y / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                const float localTrees = treeDensity[static_cast<std::size_t>(py) * DensityResolution + px];
                const float snowAccent = tdm.x > bp.snowStart - 0.05f && tdm.x < bp.snowStart
                    ? 0.025f : 0.0f;
                probability = (biome.x * (1.0f - glm::clamp(localTrees, 0.0f, 0.8f)) * 0.72f + snowAccent) *
                              (tdm.y < 0.44f ? 1.0f : 0.0f);
            }
            else if (category == 2)
            {
                const int px = std::clamp(static_cast<int>((xz.x / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                const int py = std::clamp(static_cast<int>((xz.y / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                const float localTrees = treeDensity[static_cast<std::size_t>(py) * DensityResolution + px];
                probability = biome.x * glm::mix(0.70f, 1.0f, density) *
                              (1.0f - glm::clamp(localTrees * 0.45f, 0.0f, 0.55f)) *
                              (tdm.y < 0.50f ? 0.94f : 0.0f);
            }
            else if (category == 3)
            {
                const float cluster = valueNoise(xz / 95.0f, settings_.seed + 2017u);
                const float nearWater = std::abs(surface.signedDistanceToWater) < 40.0f ? 1.0f : 0.72f;
                probability = biome.x * glm::mix(0.48f, 1.0f,
                              alpineSmoothstep(0.42f, 0.72f, cluster)) *
                              nearWater * (tdm.w > 0.55f ? 1.18f : 1.0f) *
                              (tdm.y < 0.44f ? 0.88f : 0.0f);
            }
            else
            {
                const float alpineBand = alpineSmoothstep(0.39f, 0.50f, tdm.x) *
                    (1.0f - alpineSmoothstep(0.68f, 0.76f, tdm.x));
                probability = biome.y * alpineBand * (tdm.y < 0.33f ? 1.0f : 0.0f) *
                              alpineSmoothstep(0.50f, 0.62f, tdm.w);
            }
            if (random01(attempt * 1664525u + settings_.seed + category * 733u) >
                glm::clamp(probability, 0.0f, 1.0f))
                continue;

            if (minimumSpacing > 0.0f)
            {
                const int cx = static_cast<int>(std::floor((xz.x + half) / minimumSpacing));
                const int cz = static_cast<int>(std::floor((xz.y + half) / minimumSpacing));
                bool collision = false;
                for (int dz = -1; dz <= 1 && !collision; ++dz)
                    for (int dx = -1; dx <= 1 && !collision; ++dx)
                    {
                        const std::uint64_t key = static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx + dx)) |
                            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz + dz)) << 32u);
                        const auto found = occupancy.find(key);
                        collision = found != occupancy.end() && glm::distance(found->second, xz) < minimumSpacing;
                    }
                if (collision) continue;
                const std::uint64_t key = static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) |
                    (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cz)) << 32u);
                occupancy[key] = xz;
                (void)occupancyResolution;
            }

            std::size_t variant = static_cast<std::size_t>(random01(attempt + category * 4099u) * bucketCount);
            variant = std::min(variant, bucketCount - 1u);
            if (category == 0)
            {
                const float high = alpineSmoothstep(0.24f, 0.46f, tdm.x);
                const float selector = random01(attempt * 43u + 19u);
                const float saplingChance = glm::mix(0.16f, 0.42f, high);
                const float broadChance = glm::mix(0.30f, 0.40f, high);
                variant = selector < saplingChance ? 2u :
                    (selector < saplingChance + broadChance ? 1u : 0u);
            }
            Bucket& bucket = buckets_[firstBucket + variant];
            const glm::vec3 normal = surface.normal;
            float scale = 1.0f;
            if (category == 0) scale = glm::mix(0.92f, 1.38f, random01(attempt * 61u + 3u));
            else if (category == 1) scale = glm::mix(1.20f, 1.95f, random01(attempt * 61u + 5u));
            else if (category == 2) scale = glm::mix(1.05f, 1.65f, random01(attempt * 61u + 7u));
            else if (category == 3) scale = glm::mix(1.15f, 1.75f, random01(attempt * 61u + 11u));
            else scale = glm::mix(0.78f, 1.22f, random01(attempt * 61u + 13u));
            const float sink = category == 0 ? 0.42f * scale :
                               category == 1 ? 0.18f * scale : 0.10f * scale;
            const float rotation = random01(attempt * 101u + 17u) * 2.0f * Pi;
            const float tilt = glm::clamp(std::acos(glm::clamp(normal.y, 0.0f, 1.0f)), 0.0f,
                                           category <= 1 ? 0.12f : 0.20f);
            const float tiltAzimuth = std::atan2(-normal.z, -normal.x);
            const float hue = glm::clamp(0.5f + (random01(attempt * 131u + 23u) - 0.5f) * 0.34f, 0.0f, 1.0f);
            float value = glm::mix(0.86f, 1.14f, random01(attempt * 151u + 29u));
            std::uint8_t flags = 0u;
            if (category == 2 && random01(attempt * 173u + 31u) < 0.10f)
            {
                value = 0.72f;
                flags |= 1u;
            }
            const std::uint8_t shape = static_cast<std::uint8_t>(random01(attempt * 181u + 37u) * 255.0f);
            const std::uint32_t packed = packVariation(hue, glm::clamp((value - 0.65f) / 0.70f, 0.0f, 1.0f), shape, flags);
            bucket.instances.push_back({glm::vec4(xz.x, surface.worldPosition.y - sink, xz.y, scale),
                                        glm::vec4(rotation, tilt, tiltAzimuth, static_cast<float>(packed))});
            ++accepted;
            if (category == 0)
            {
                const int px = std::clamp(static_cast<int>((xz.x / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                const int py = std::clamp(static_cast<int>((xz.y / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
                treeDensity[static_cast<std::size_t>(py) * DensityResolution + px] =
                    std::min(1.0f, treeDensity[static_cast<std::size_t>(py) * DensityResolution + px] + 0.12f);
            }
        }
        if (accepted < budget)
            std::cout << "Vegetation category " << category << " accepted " << accepted
                      << " / " << budget << std::endl;
    };

    addCategory(0, 3, settings_.treeBudget, 6.0f, 0);
    addCategory(3, 2, settings_.shrubBudget, 3.0f, 1);
    addCategory(5, 3, settings_.grassBudget, 0.0f, 2);
    addCategory(8, 3, settings_.flowerBudget, 0.0f, 3);
    addCategory(11, 2, settings_.cushionBudget, 1.6f, 4);
    for (Bucket& bucket : buckets_)
        buildChunks(bucket);
    const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Vegetation distribution generated in " << seconds << " s" << std::endl;
}

void AlpineVegetationSystem::buildChunks(Bucket& bucket)
{
    const float size = terrain_.getSettings().size;
    const int chunksPerAxis = static_cast<int>(std::ceil(size / settings_.chunkSize));
    const auto key = [&](const Instance& instance)
    {
        const int x = std::clamp(static_cast<int>((instance.posScale.x + size * 0.5f) / settings_.chunkSize), 0, chunksPerAxis - 1);
        const int z = std::clamp(static_cast<int>((instance.posScale.z + size * 0.5f) / settings_.chunkSize), 0, chunksPerAxis - 1);
        return z * chunksPerAxis + x;
    };
    std::stable_sort(bucket.instances.begin(), bucket.instances.end(),
                     [&](const Instance& a, const Instance& b) { return key(a) < key(b); });
    bucket.chunks.clear();
    for (std::size_t first = 0; first < bucket.instances.size();)
    {
        const int k = key(bucket.instances[first]);
        std::size_t end = first + 1u;
        while (end < bucket.instances.size() && key(bucket.instances[end]) == k) ++end;
        const int x = k % chunksPerAxis;
        const int z = k / chunksPerAxis;
        bucket.chunks.push_back({static_cast<std::uint32_t>(first),
            static_cast<std::uint32_t>(end - first),
            glm::vec2(-size * 0.5f + (x + 0.5f) * settings_.chunkSize,
                      -size * 0.5f + (z + 0.5f) * settings_.chunkSize)});
        first = end;
    }
}

void AlpineVegetationSystem::buildDensityTexture()
{
    std::vector<float> density(static_cast<std::size_t>(DensityResolution) * DensityResolution, 0.0f);
    const float size = terrain_.getSettings().size;
    for (std::size_t b = 5; b <= 7; ++b)
        for (const Instance& instance : buckets_[b].instances)
        {
            const int x = std::clamp(static_cast<int>((instance.posScale.x / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
            const int z = std::clamp(static_cast<int>((instance.posScale.z / size + 0.5f) * DensityResolution), 0, DensityResolution - 1);
            density[static_cast<std::size_t>(z) * DensityResolution + x] += 1.0f;
        }
    std::vector<float> blurred(density.size(), 0.0f);
    float maximum = 1.0f;
    for (int z = 0; z < DensityResolution; ++z)
        for (int x = 0; x < DensityResolution; ++x)
        {
            float sum = 0.0f;
            float weight = 0.0f;
            for (int dz = -2; dz <= 2; ++dz)
                for (int dx = -2; dx <= 2; ++dx)
                {
                    const int sx = std::clamp(x + dx, 0, DensityResolution - 1);
                    const int sz = std::clamp(z + dz, 0, DensityResolution - 1);
                    const float w = 1.0f / (1.0f + static_cast<float>(dx * dx + dz * dz));
                    sum += density[static_cast<std::size_t>(sz) * DensityResolution + sx] * w;
                    weight += w;
                }
            blurred[static_cast<std::size_t>(z) * DensityResolution + x] = sum / weight;
            maximum = std::max(maximum, sum / weight);
        }
    densityPixels_.resize(blurred.size());
    for (std::size_t i = 0; i < blurred.size(); ++i)
        densityPixels_[i] = static_cast<std::uint8_t>(glm::clamp(blurred[i] / maximum, 0.0f, 1.0f) * 255.0f);
}

void AlpineVegetationSystem::upload()
{
    const auto uploadMesh = [](GpuMesh& gpu, const VegetationMeshData& cpu, GLuint instanceBuffer)
    {
        if (cpu.vertices.empty()) return;
        glGenVertexArrays(1, &gpu.vao);
        glGenBuffers(1, &gpu.vertexBuffer);
        glBindVertexArray(gpu.vao);
        glBindBuffer(GL_ARRAY_BUFFER, gpu.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpu.vertices.size() * sizeof(VegetationVertex)), cpu.vertices.data(), GL_STATIC_DRAW);
        if (!cpu.indices.empty())
        {
            glGenBuffers(1, &gpu.indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpu.indices.size() * sizeof(std::uint16_t)), cpu.indices.data(), GL_STATIC_DRAW);
        }
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VegetationVertex), reinterpret_cast<void*>(offsetof(VegetationVertex, position)));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VegetationVertex), reinterpret_cast<void*>(offsetof(VegetationVertex, normal)));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VegetationVertex), reinterpret_cast<void*>(offsetof(VegetationVertex, colorRoughness)));
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VegetationVertex), reinterpret_cast<void*>(offsetof(VegetationVertex, windVariation)));
        glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
        glEnableVertexAttribArray(5); glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), nullptr); glVertexAttribDivisor(5, 1);
        glEnableVertexAttribArray(6); glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(sizeof(glm::vec4))); glVertexAttribDivisor(6, 1);
        gpu.indexCount = static_cast<GLsizei>(cpu.indices.size());
        glBindVertexArray(0);
    };
    for (Bucket& bucket : buckets_)
    {
        glGenBuffers(1, &bucket.instanceBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, bucket.instanceBuffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bucket.instances.size() * sizeof(Instance)), bucket.instances.data(), GL_STATIC_DRAW);
        uploadMesh(bucket.meshes[0], bucket.cpuMeshes.lod0, bucket.instanceBuffer);
        uploadMesh(bucket.meshes[1], bucket.cpuMeshes.lod1, bucket.instanceBuffer);
        uploadMesh(bucket.meshes[2], bucket.cpuMeshes.lod2, bucket.instanceBuffer);
        uploadMesh(bucket.meshes[3], bucket.cpuMeshes.shadow, bucket.instanceBuffer);

        // One point vertex per species. The far pass instances this vertex for
        // the complete bucket and rejects near instances in the vertex shader.
        VegetationMeshData point;
        const VegetationMeshData& source = !bucket.cpuMeshes.lod2.vertices.empty()
            ? bucket.cpuMeshes.lod2 : bucket.cpuMeshes.lod0;
        glm::vec3 averageColor(0.08f, 0.18f, 0.04f);
        float averageRoughness = 0.9f;
        if (!source.vertices.empty())
        {
            averageColor = glm::vec3(0.0f);
            averageRoughness = 0.0f;
            for (const VegetationVertex& vertex : source.vertices)
            {
                averageColor += glm::vec3(vertex.colorRoughness);
                averageRoughness += vertex.colorRoughness.w;
            }
            const float inverseCount = 1.0f / static_cast<float>(source.vertices.size());
            averageColor *= inverseCount;
            averageRoughness *= inverseCount;
        }
        point.vertices.push_back({glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                  glm::vec4(averageColor, averageRoughness),
                                  glm::vec2(0.0f)});
        uploadMesh(bucket.meshes[4], point, bucket.instanceBuffer);
    }
    glGenTextures(1, &densityTexture_);
    glBindTexture(GL_TEXTURE_2D, densityTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, DensityResolution, DensityResolution, 0,
                 GL_RED, GL_UNSIGNED_BYTE, densityPixels_.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AlpineVegetationSystem::destroyGpuResources()
{
    for (Bucket& bucket : buckets_)
    {
        for (GpuMesh& mesh : bucket.meshes)
        {
            if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
            if (mesh.vertexBuffer) glDeleteBuffers(1, &mesh.vertexBuffer);
            if (mesh.indexBuffer) glDeleteBuffers(1, &mesh.indexBuffer);
        }
        if (bucket.instanceBuffer) glDeleteBuffers(1, &bucket.instanceBuffer);
    }
    if (densityTexture_) glDeleteTextures(1, &densityTexture_);
}

void AlpineVegetationSystem::beginFrame(const Camera& camera, int width, int height)
{
    previousViewProjection_ = frameInitialized_ ? currentViewProjection_ : glm::mat4(1.0f);
    previousWindTime_ = frameInitialized_ ? currentWindTime_ : elapsedSeconds();
    currentWindTime_ = elapsedSeconds();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        static_cast<float>(std::max(width, 1)) / static_cast<float>(std::max(height, 1)), 0.1f, 20000.0f);
    projection = TemporalJitter::apply(projection, width, height);
    currentViewProjection_ = projection * camera.GetViewMatrix();
    if (!frameInitialized_) previousViewProjection_ = currentViewProjection_;
    frameCameraPosition_ = camera.Getposition();
    frameViewportHeight_ = std::max(height, 1);
    frameInitialized_ = true;
}

void AlpineVegetationSystem::drawGeometry(Shader& shader, const Camera& camera,
                                          const SceneRenderConfig& config) const
{
    drawBuckets(shader, camera, config, false);
}

void AlpineVegetationSystem::drawDirectionalShadow(Shader& shader, const Camera& camera,
                                                   const SceneRenderConfig& config) const
{
    drawBuckets(shader, camera, config, true);
}

void AlpineVegetationSystem::drawBuckets(Shader& shader, const Camera& camera,
                                         const SceneRenderConfig& config, bool shadow) const
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthRange(0.0, 1.0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    shader.use();
    shader.setMat4("u_viewProjection", currentViewProjection_);
    shader.setMat4("u_previousViewProjection", previousViewProjection_);
    shader.setFloat("u_time", currentWindTime_);
    shader.setFloat("u_previousTime", previousWindTime_);
    shader.setFloat("u_windSpeed", config.vegetationWindSpeed);
    shader.setFloat("u_windStrength", config.vegetationWindStrength);
    const float windLength = glm::length(config.vegetationWindDirection);
    shader.setVec2("u_windDir", windLength > 0.0001f
        ? config.vegetationWindDirection / windLength : glm::vec2(1.0f, 0.0f));
    shader.setVec3("u_cameraPosition", camera.Getposition());
    if (!shadow)
        shader.setBool("u_pointMode", false);
    struct DrawCommand
    {
        std::size_t bucket;
        std::size_t chunk;
        int lod;
        float distance;
        float maximumDistance;
    };
    std::array<std::vector<DrawCommand>, static_cast<std::size_t>(Species::Count)> visible;
    for (std::size_t b = 0; b < buckets_.size(); ++b)
    {
        const Bucket& bucket = buckets_[b];
        float maximumDistance = isTree(b) ? (shadow ? 300.0f : config.vegetationTreeDistance) :
            isShrub(b) ? 320.0f : isGrass(b) ? config.vegetationGrassDistance :
            isFlower(b) ? config.vegetationFlowerDistance : 240.0f;
        if (shadow && !isTree(b)) continue;
        for (std::size_t c = 0; c < bucket.chunks.size(); ++c)
        {
            const Chunk& chunk = bucket.chunks[c];
            const float distance = glm::distance(glm::vec2(frameCameraPosition_.x, frameCameraPosition_.z), chunk.center) -
                                   settings_.chunkSize * 0.70710678f;
            if (distance > maximumDistance) continue;
            int lod = shadow ? 3 : 0;
            if (!shadow)
            {
                if (isTree(b)) lod = distance > 220.0f ? 2 : (distance > 80.0f ? 1 : 0);
                else if (isShrub(b)) lod = distance > 140.0f ? 2 : (distance > 50.0f ? 1 : 0);
                else if (isGrass(b)) lod = distance > 70.0f ? 2 : (distance > 25.0f ? 1 : 0);
                else if (isFlower(b)) lod = distance > 45.0f ? 1 : 0;
                else lod = distance > 100.0f ? 2 : (distance > 40.0f ? 1 : 0);
            }
            if (bucket.meshes[static_cast<std::size_t>(lod)].indexCount > 0)
                visible[b].push_back({b, c, lod, distance, maximumDistance});
        }
        std::sort(visible[b].begin(), visible[b].end(),
                  [](const DrawCommand& a, const DrawCommand& c)
                  { return a.distance < c.distance; });
    }

    std::vector<DrawCommand> selected;
    constexpr std::size_t NearGeometryDrawBudget = 47u;
    selected.reserve(NearGeometryDrawBudget);
    std::vector<DrawCommand> remainder;
    for (std::size_t b = 0; b < visible.size(); ++b)
    {
        // Fair per-species reservation prevents the first tree bucket from
        // starving all grass, flower and variant buckets under the 60-call cap.
        const std::size_t quota = shadow ? 20u :
            (isTree(b) ? 3u : isShrub(b) ? 2u : isGrass(b) ? 5u :
             isFlower(b) ? 4u : 1u);
        const std::size_t guaranteed = std::min(quota, visible[b].size());
        selected.insert(selected.end(), visible[b].begin(),
                        visible[b].begin() + guaranteed);
        remainder.insert(remainder.end(), visible[b].begin() + guaranteed,
                         visible[b].end());
    }
    std::sort(remainder.begin(), remainder.end(), [](const DrawCommand& a,
                                                      const DrawCommand& b)
    {
        const float aScore = a.distance / std::max(a.maximumDistance, 1.0f);
        const float bScore = b.distance / std::max(b.maximumDistance, 1.0f);
        return aScore < bScore;
    });
    const std::size_t drawBudget = shadow ? 60u : NearGeometryDrawBudget;
    if (selected.size() < drawBudget)
    {
        const std::size_t extra = std::min<std::size_t>(drawBudget - selected.size(),
                                                         remainder.size());
        selected.insert(selected.end(), remainder.begin(), remainder.begin() + extra);
    }
    if (selected.size() > drawBudget)
        selected.resize(drawBudget);

    for (const DrawCommand& command : selected)
    {
        const std::size_t b = command.bucket;
        const Bucket& bucket = buckets_[b];
        const Chunk& chunk = bucket.chunks[command.chunk];
        const GpuMesh& mesh = bucket.meshes[static_cast<std::size_t>(command.lod)];
        if (isGrass(b) || isFlower(b)) glDisable(GL_CULL_FACE);
        else glEnable(GL_CULL_FACE);
        std::uint32_t count = chunk.count;
        if (!shadow && command.distance > command.maximumDistance * 0.72f &&
            (isGrass(b) || isFlower(b)))
            count = std::max(1u, count / 2u);
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, bucket.instanceBuffer);
        const std::size_t byteOffset = static_cast<std::size_t>(chunk.first) * sizeof(Instance);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(byteOffset));
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(byteOffset + sizeof(glm::vec4)));
        glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT,
                                nullptr, static_cast<GLsizei>(count));
    }

    if (!shadow)
    {
        // One point-sprite draw per species keeps the entire terrain resident
        // and visible. The vertex shader removes the near range, so these
        // points never overlap their geometric LODs and never evaluate wind.
        static constexpr std::array<float, 13> PointWorldHeights = {{
            48.0f, 36.0f, 20.0f, 3.6f, 3.2f,
            3.2f, 3.2f, 3.2f, 4.2f, 4.2f, 4.8f, 1.5f, 1.5f}};
        static constexpr std::array<float, 13> PointMaximumPixels = {{
            12.0f, 11.0f, 8.0f, 4.0f, 4.0f,
            2.5f, 2.5f, 2.5f, 3.5f, 3.5f, 4.0f, 2.0f, 2.0f}};
        shader.setBool("u_pointMode", true);
        shader.setFloat("u_pointPixelScale",
                        static_cast<float>(frameViewportHeight_) * 1.20710678f);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDisable(GL_CULL_FACE);
        for (std::size_t b = 0; b < buckets_.size(); ++b)
        {
            const Bucket& bucket = buckets_[b];
            const GpuMesh& point = bucket.meshes[4];
            if (!point.vao || bucket.instances.empty())
                continue;
            const float nearDistance = isTree(b) ? config.vegetationTreeDistance :
                isShrub(b) ? 320.0f : isGrass(b) ? config.vegetationGrassDistance :
                isFlower(b) ? config.vegetationFlowerDistance : 240.0f;
            shader.setFloat("u_pointMinDistance", nearDistance);
            shader.setFloat("u_pointWorldHeight", PointWorldHeights[b]);
            shader.setFloat("u_pointMaxPixels", PointMaximumPixels[b]);
            glBindVertexArray(point.vao);
            glDrawArraysInstanced(GL_POINTS, 0, 1,
                                  static_cast<GLsizei>(bucket.instances.size()));
        }
        shader.setBool("u_pointMode", false);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

void AlpineVegetationSystem::bindTerrainDensity(Shader& shader, int textureUnit) const
{
    shader.setBool("hasVegetationDensity", densityTexture_ != 0);
    shader.setInt("vegetationDensityMap", textureUnit);
    shader.setFloat("vegetationTerrainSize", terrain_.getSettings().size);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, densityTexture_);
}

std::size_t AlpineVegetationSystem::instanceCount() const
{
    return std::accumulate(buckets_.begin(), buckets_.end(), std::size_t{0},
        [](std::size_t total, const Bucket& bucket) { return total + bucket.instances.size(); });
}

std::uint64_t AlpineVegetationSystem::settingsHash() const
{
    std::uint64_t hash = 1469598103934665603ull;
    const auto append = [&hash](const void* data, std::size_t count)
    {
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < count; ++i)
        {
            hash ^= bytes[i];
            hash *= 1099511628211ull;
        }
    };
    append(&CacheVersion, sizeof(CacheVersion));
    append(&settings_.seed, sizeof(settings_.seed));
    append(&settings_.treeBudget, sizeof(settings_.treeBudget));
    append(&settings_.shrubBudget, sizeof(settings_.shrubBudget));
    append(&settings_.grassBudget, sizeof(settings_.grassBudget));
    append(&settings_.flowerBudget, sizeof(settings_.flowerBudget));
    append(&settings_.cushionBudget, sizeof(settings_.cushionBudget));
    append(&settings_.chunkSize, sizeof(settings_.chunkSize));
    return hash;
}

bool AlpineVegetationSystem::loadCache()
{
    const auto path = cacheDirectory() / ("alpine_" + std::to_string(terrain_.getParameterHash()) +
                                           "_" + std::to_string(settingsHash()) + ".bin");
    std::ifstream stream(path, std::ios::binary);
    CacheHeader header{};
    if (!stream.read(reinterpret_cast<char*>(&header), sizeof(header)) ||
        header.magic != CacheMagic || header.version != CacheVersion ||
        header.terrainHash != terrain_.getParameterHash() || header.settingsHash != settingsHash() ||
        header.speciesCount != buckets_.size()) return false;
    for (Bucket& bucket : buckets_)
    {
        std::uint32_t count = 0;
        if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count))) return false;
        bucket.instances.resize(count);
        if (count && !stream.read(reinterpret_cast<char*>(bucket.instances.data()),
                                  static_cast<std::streamsize>(count * sizeof(Instance)))) return false;
    }
    return true;
}

void AlpineVegetationSystem::saveCache() const
{
    std::error_code error;
    std::filesystem::create_directories(cacheDirectory(), error);
    const auto path = cacheDirectory() / ("alpine_" + std::to_string(terrain_.getParameterHash()) +
                                           "_" + std::to_string(settingsHash()) + ".bin");
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    const CacheHeader header{CacheMagic, CacheVersion, terrain_.getParameterHash(), settingsHash(),
                             static_cast<std::uint32_t>(buckets_.size()), 0u};
    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    for (const Bucket& bucket : buckets_)
    {
        const std::uint32_t count = static_cast<std::uint32_t>(bucket.instances.size());
        stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (count) stream.write(reinterpret_cast<const char*>(bucket.instances.data()),
                                static_cast<std::streamsize>(count * sizeof(Instance)));
    }
}

void AlpineVegetationSystem::validateBiomeParity() const
{
    const AlpineBiomeParameters p;
    float maximumError = 0.0f;
    for (std::uint32_t i = 0; i < 1000u; ++i)
    {
        const float h = random01(i * 3u + 1u);
        const float a = random01(i * 3u + 2u);
        const float n = random01(i * 3u + 3u);
        const glm::vec3 cpu = alpineBiomeWeights(h, a, n, p);
        const float sunFacing = std::cos(a * 2.0f * Pi - p.sunAzimuth);
        const float shifted = h - sunFacing * p.sunHeightShift - (n - 0.5f) * p.noiseHeightShift;
        const float gtr = alpineSmoothstep(p.grassEnd, p.rockStart, shifted);
        const float rts = alpineSmoothstep(p.snowStart, p.snowEnd, shifted);
        const glm::vec3 glslReference(1.0f - gtr, gtr * (1.0f - rts), gtr * rts);
        const glm::vec3 error = glm::abs(cpu - glslReference);
        maximumError = std::max(maximumError, std::max(error.x, std::max(error.y, error.z)));
    }
    if (maximumError >= 1e-3f)
        std::cerr << "ERROR::VEGETATION::CPU/GLSL biome parity failed: " << maximumError << std::endl;
}
