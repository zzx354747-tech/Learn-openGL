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
    bool enableTAA = true;
    bool enableProceduralSky = true;
    bool enableVolumetricClouds = true;
    bool enableSunTexture = true;
    bool enableGodRays = true;
    float taaHistoryWeight = 0.88f;
    float taaSharpness = 0.28f;
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
    float stormHoleSize = 1100.0f;
    float stormPoolHoleSize = 38.0f;
    float stormHoleSpacing = 32000.0f;
    float stormHoleSoftness = 0.34f;
    float stormHoleShaftStrength = 2.0f;
    glm::vec2 stormHeroHolePosition = glm::vec2(0.0f, -18.0f);
    glm::vec2 stormShaftLean = glm::vec2(0.08f, 0.0f);
    float cloudSpeed = 7.0f;
    float cloudEvolutionSpeed = 0.035f;
    float cloudEvolutionPhase = 0.0f;
    glm::vec2 cloudAnimationOffset = glm::vec2(0.0f);
    glm::vec2 cloudWindDirection = glm::vec2(0.94f, 0.34f);
    float cloudWindShear = 0.08f;
    float cloudExtinction = 1.05f;
    float cloudLightAbsorption = 0.90f;
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
    float godRayIntensity = 0.82f;
    float godRayDensity = 0.90f;
    float godRayDecay = 0.967f;
    float godRayWeight = 0.058f;
    float godRayExposure = 0.82f;
    float godRayRadius = 0.38f;
    int godRaySamples = 48;
    glm::vec3 godRayColor = glm::vec3(1.0f, 0.78f, 0.50f);
    float sunAngularRadius = 0.075f;
    float ssaoStrength = 1.0f;
    float giStrength = 0.85f;
    float giRadius = 18.0f;
    float giMaxDistance = 8.0f;
    int giSampleCount = 12;
    glm::vec3 fixedAmbientColor = glm::vec3(0.08f);
    float fixedAmbientStrength = 1.0f;
    glm::vec3 iblAmbientTint = glm::vec3(1.0f);
    float iblAmbientStrength = 1.0f;
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
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
