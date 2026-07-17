#pragma once
#include <glm/glm.hpp>
#include <cmath>

// 编译期常量，无法在运行时修改
constexpr unsigned int MaterialSphereCount = 8;

enum class RenderMode
{
    Basic,
    Lighting,
    Reflection,
    ShadowDebug,
};

enum class ForwardLightMode { 
    Basic, 
    Reflect, 
    Light };

enum class SceneSelection
{
    Default,
    LivingRoom,
    FujiTerrain,
};

enum class EnvironmentSelection
{
    Night,
    Sunny,
    GodRays,
    NightN8_3K,
};

enum class CloudWeatherPreset
{
    Storm,
    Sunny,
    Overcast,
};

struct WaterRenderSettings
{
    // Default look: clear, sunlit tropical-island water.
    glm::vec2 windDirection = glm::vec2(0.91f, 0.41f);
    float waveAmplitude = 0.075f;
    float wavelengthScale = 1.18f;
    float detailNormalStrength = 0.18f;
    float refractionStrength = 0.024f;
    glm::vec3 absorptionCoefficient = glm::vec3(0.10f, 0.035f, 0.018f);
    glm::vec3 scatteringColor = glm::vec3(0.025f, 0.24f, 0.30f);
    float maxAbsorptionDistance = 42.0f;
    float roughness = 0.055f;
    float foamShoreWidth = 0.78f;

    bool enableCaustics = true;
    float causticStrength = 2.20f;
    float causticScale = 0.15f;
    float causticSharpness = 2.6f;
    float causticCurvatureScale = 0.52f;
    float causticDepthStart = 0.05f;
    float causticDepthPeak = 2.0f;
    float causticDepthEnd = 120.0f;
    float causticAbsorptionScale = 0.10f;

    bool enableDispersion = true;
    glm::vec3 iorRGB = glm::vec3(1.331f, 1.333f, 1.337f);
    float dispersionStrength = 0.85f;
    float dispersionBlend = 0.72f;
    float dispersionDepthFalloff = 0.035f;
    float dispersionMaxPixels = 4.5f;
    float spectralGlintStrength = 0.028f;
};

struct SceneRenderConfig
{
    bool enableSkybox = false;
    bool enableFlashlight = false;
    bool enablePointLight = false;
    bool enableDirectionalLight = false;
    bool enableGammaCorrection = false;
    bool enableHDR = false;
    bool enableBloom = false;
    bool enableSSAO = false;
    bool enablePBR = false;
    bool enableIBL = false;
    bool enableGI = true;
    bool enableWater = true;
    bool enableVegetation = true;
    bool enableTAA = true;
    bool enableProceduralSky = true;
    bool enableVolumetricClouds = true;
    bool enableSunTexture = true;
    bool enableGodRays = true;
    bool enableTimeOfDay = true;
    float timeOfDayHours = 9.0f;
    float dayLengthSeconds = 360.0f;
    float daylightFactor = 1.0f;
    bool enableAutomaticWeather = true;
    float automaticWeatherIntervalSeconds = 90.0f;
    WaterRenderSettings water;
    float taaHistoryWeight = 0.88f;
    float taaSharpness = 0.28f;
    glm::vec2 vegetationWindDirection = glm::vec2(0.88f, 0.47f);
    float vegetationWindSpeed = 1.15f;
    float vegetationWindStrength = 0.32f;
    float vegetationGrassDistance = 150.0f;
    float vegetationFlowerDistance = 100.0f;
    float vegetationTreeDistance = 1400.0f;
    glm::vec3 skyTopColor = glm::vec3(0.24f, 0.55f, 0.90f);
    CloudWeatherPreset cloudWeatherPreset = CloudWeatherPreset::Sunny;
    unsigned int cloudWeatherTransitionRequest = 0;
    float cloudWeatherTransitionDuration = 18.0f;
    float cloudWeatherTransitionProgress = 1.0f;
    float cloudCoverage = 0.32f;
    float cloudDensity = 0.92f;
    float cloudBaseHeight = 1500.0f;
    float cloudThickness = 1050.0f;
    float cloudScale = 0.82f;
    float cloudDetailScale = 3.80f;
    float cloudType = 0.78f;
    float cloudAnvilAmount = 0.18f;
    float cloudErosionStrength = 0.28f;
    float stormHoleStrength = 0.0f;
    unsigned int stormHoleSeed = 1739u;
    // World-space center of the primary storm aperture at cloud mid-height.
    // It is captured when the pattern is generated and never follows camera.
    glm::vec2 stormHoleAnchor = glm::vec2(0.0f, -8000.0f);
    int stormHoleCount = 5;
    float stormHoleMinRadius = 160.0f;
    float stormHoleMaxRadius = 1400.0f;
    float stormHoleSoftness = 0.38f;
    // Independent lens-facing solar glare intensity. The historical member
    // name is retained to avoid widening this renderer configuration change.
    float stormHoleShaftStrength = 1.0f;
    float cloudSpeed = 7.0f;
    float cloudEvolutionSpeed = 0.035f;
    float cloudEvolutionPhase = 0.0f;
    glm::vec2 cloudAnimationOffset = glm::vec2(0.0f);
    glm::vec2 cloudWindDirection = glm::vec2(0.94f, 0.34f);
    float cloudWindShear = 0.08f;
    float cloudExtinction = 1.05f;
    float cloudLightAbsorption = 3.50f;
    float cloudShadowStrength = 1.0f;
    float cloudShadowCoverage = 8000.0f;
    int cloudShadowMarchSteps = 6;
    int cloudShadowScanSlices = 4;
    float cloudAmbientStrength = 0.72f;
    float cloudPowderStrength = 1.35f;
    float cloudMultiScattering = 0.48f;
    float cloudSilverLining = 1.85f;
    float cloudForwardScattering = 0.68f;
    float cloudBackwardScattering = -0.20f;
    glm::vec3 cloudSunColor = glm::vec3(1.0f, 0.91f, 0.70f);
    glm::vec3 cloudBottomColor = glm::vec3(0.34f, 0.46f, 0.63f);
    glm::vec3 cloudTopColor = glm::vec3(0.84f, 0.91f, 1.0f);
    int cloudViewSteps = 48;
    int cloudLightSteps = 5;
    float cloudMaxDistance = 85000.0f;
    float cloudRenderScale = 0.5f;
    bool enableCloudAcceleration = true;
    bool enableCloudOccupancySkipping = true;
    bool enableCloudLightCache = true;
    int cloudCacheResolution = 256;
    int cloudCacheUpdateInterval = 1;
    int cloudCacheLightSteps = 4;
    float cloudCacheWorldSize = 180000.0f;
    bool enableSunLocalCloudCache = true;
    int sunLocalCloudCacheResolution = 512;
    float sunLocalCloudCacheWorldSize = 28000.0f;
    int sunLocalCloudCacheTilesPerAxis = 2;
    float cloudOccupancyThreshold = 0.012f;
    float cloudEmptySkipMultiplier = 6.0f;
    float godRayIntensity = 0.82f;
    float godRayDensity = 0.90f;
    float godRayDecay = 0.967f;
    float godRayWeight = 0.058f;
    float godRayExposure = 0.82f;
    float godRayRadius = 0.38f;
    int godRaySamples = 48;
    glm::vec3 godRayColor = glm::vec3(1.0f, 0.78f, 0.50f);
    float volumetricLightRenderScale = 0.5f;
    int volumetricLightSteps = 32;
    float volumetricLightAnisotropy = 0.68f;
    // -ln(0.30) / 500 m: approximately 30% transmittance after 500 metres.
    float volumetricLightExtinction = 0.0024079f;
    float volumetricLightScattering = 0.0012f;
    float volumetricLightMaxDistance = 8000.0f;
    float volumetricLightIntensity = 1.0f;
    float volumetricLightDepthSigma = 0.012f;
    float sunAngularRadius = 0.075f;
    float ssaoStrength = 1.0f;
    float giStrength = 0.85f;
    float giRadius = 18.0f;
    float giMaxDistance = 8.0f;
    int giSampleCount = 12;
    glm::vec3 fixedAmbientColor = glm::vec3(0.08f);
    float fixedAmbientStrength = 0.15f;
    glm::vec3 iblAmbientTint = glm::vec3(1.0f);
    float iblAmbientStrength = 0.15f;
    float phongDiffuseStrength = 0.55f;
    float phongSpecularStrength = 0.18f;
    float phongIBLDiffuseStrength = 1.25f;
    float phongIBLSpecularStrength = 0.35f;

    int numBlurPasses = 10;
    float exposure = 1.0f;
    float bloomStrength = 0.6f;
    float bloomThreshold = 0.7f;
    float sunThreshold = 100.0f;
    float directionalShadowLightSize = 0.006f;
    float directionalShadowBlockerSearchRadius = 0.006f;
    float directionalShadowMinFilterRadius = 0.0005f;
    float directionalShadowMaxFilterRadius = 0.014f;
    float directionalShadowBiasSlope = 0.005f;
    float directionalShadowBiasMin = 0.0005f;

    // Runtime terrain material controls. Geometry and derived fields are
    // immutable after startup; these values only classify/sample the TDM.
    float terrainSunAzimuth = 0.0f;
    float terrainSunHeightShift = 0.06f;
    float terrainNoiseHeightShift = 0.08f;
    // Low plains remain grass. Only the mountain transition is lowered and
    // narrowed so exposed rock occupies more of the lower mountain body.
    float terrainGrassEnd = 0.36f;
    float terrainRockStart = 0.39f;
    // Calibrated against the generated terrain surface: weighted snow cover
    // remains above 30% of samples classified as mountain rather than merely
    // occupying 30% of the vertical elevation range.
    float terrainSnowStart = 0.53f;
    float terrainSnowEnd = 0.56f;
    float terrainBlendSharpness = 0.12f;
    float terrainTextureScale = 1.0f / 64.0f;
    int terrainDebugMode = 0;

    RenderMode renderMode = RenderMode::Basic;
    ForwardLightMode forwardLightMode = ForwardLightMode::Basic;
    SceneSelection sceneSelection = SceneSelection::Default;
    EnvironmentSelection environmentSelection = EnvironmentSelection::Night;
};

inline bool shouldRenderVolumetricClouds(const SceneRenderConfig& config)
{
    return config.renderMode == RenderMode::Lighting &&
           config.enableVolumetricClouds;
}

inline bool shouldRenderGodRays(const SceneRenderConfig& config)
{
    return config.renderMode == RenderMode::Lighting &&
           config.enableGodRays;
}

inline float calculateCloudSunTransmission(const SceneRenderConfig& config)
{
    if (!shouldRenderVolumetricClouds(config))
        return 1.0f;

    const float coverage = glm::smoothstep(
        0.15f, 0.95f, glm::clamp(config.cloudCoverage, 0.0f, 1.0f));
    const float densityTransmission = std::exp(
        -glm::max(config.cloudDensity, 0.0f) * 0.38f);
    const float cloudTransmission = glm::mix(
        1.0f, densityTransmission, coverage);
    const float stormTransmission = glm::mix(
        1.0f, 0.12f, glm::clamp(config.stormHoleStrength, 0.0f, 1.0f));
    return glm::clamp(
        cloudTransmission * stormTransmission, 0.04f, 1.0f);
}

inline float calculateCloudAmbientTransmission(const SceneRenderConfig& config)
{
    if (!shouldRenderVolumetricClouds(config))
        return 1.0f;

    const float coverage = glm::smoothstep(
        0.15f, 0.95f, glm::clamp(config.cloudCoverage, 0.0f, 1.0f));
    const float cloudDimmer = glm::mix(1.0f, 0.55f, coverage);
    const float stormDimmer = glm::mix(
        1.0f, 0.14f, glm::clamp(config.stormHoleStrength, 0.0f, 1.0f));
    return glm::clamp(cloudDimmer * stormDimmer, 0.06f, 1.0f);
}

struct SceneRenderState
{
    glm::vec3 materialSpherePositions[MaterialSphereCount] =
    {
        glm::vec3(-2.4f, 0.0f, -3.9f),
        glm::vec3(-0.8f, 0.0f, -3.9f),
        glm::vec3( 0.8f, 0.0f, -3.9f),
        glm::vec3( 2.4f, 0.0f, -3.9f),
        glm::vec3(-2.4f, 0.0f, -5.7f),
        glm::vec3(-0.8f, 0.0f, -5.7f),
        glm::vec3( 0.8f, 0.0f, -5.7f),
        glm::vec3( 2.4f, 0.0f, -5.7f),
    };

    glm::vec3 lightPositions = glm::vec3(-1.0f, 9.0f, -5.0f);
    glm::vec3 color = glm::vec3(1.0f);

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 cloudShadowMatrix = glm::mat4(1.0f);
    float cloudShadowGlobalTransmission = 1.0f;
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
