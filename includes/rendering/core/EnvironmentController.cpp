#include "rendering/core/EnvironmentController.h"
#include "rendering/resources/environment/EnvironmentOption.h"

#include <iostream>

EnvironmentController::EnvironmentController(
    SceneRenderConfig& sceneConfig,
    RenderParams& renderParams,
    LightSettings& lightSettings,
    SceneRenderResources& sceneResources,
    Shader& irradianceShader,
    Shader& prefilterShader
)
    : sceneConfig_(sceneConfig),
      renderParams_(renderParams),
      lightSettings_(lightSettings),
      sceneResources_(sceneResources),
      irradianceShader_(irradianceShader),
      prefilterShader_(prefilterShader)
{
}

bool EnvironmentController::load()
{
    int environmentIndex = getEnvironmentIndex(sceneConfig_.environmentSelection);
    HDRLoadOptions loadOptions = kEnvironmentOptions[environmentIndex].loadOptions;
    loadOptions.sunThreshold = sceneConfig_.sunThreshold;

    auto nextHdrTexture = std::make_unique<HDRTexture>();
    if (!nextHdrTexture->load(kEnvironmentOptions[environmentIndex].path, loadOptions))
    {
        std::cerr << "Failed to switch environment: "
                  << kEnvironmentOptions[environmentIndex].name << std::endl;
        return false;
    }

    auto nextSkybox = std::make_unique<EnvCubemap>(
        *nextHdrTexture,
        sceneResources_.shaderLibrary->envCubemap);
    auto nextIrradianceMap = std::make_unique<IrradianceMap>(
        *nextSkybox,
        irradianceShader_);
    auto nextPrefilterMap = std::make_unique<PrefilterMap>(
        *nextSkybox,
        prefilterShader_);

    if (!nextSkybox->isReady() ||
        !nextIrradianceMap->isReady() ||
        !nextPrefilterMap->isReady())
    {
        std::cerr << "Failed to build environment cubemaps: "
                  << kEnvironmentOptions[environmentIndex].name << std::endl;
        return false;
    }

    hdrTexture_ = std::move(nextHdrTexture);
    skybox_ = std::move(nextSkybox);
    irradianceMap_ = std::move(nextIrradianceMap);
    prefilterMap_ = std::move(nextPrefilterMap);

    sceneResources_.skybox = skybox_.get();
    sceneResources_.irradianceMap = irradianceMap_.get();
    sceneResources_.prefilterMap = prefilterMap_.get();
    sceneResources_.registry.setTexture(
        sceneResources_.lightingHandles.irradianceMap,
        irradianceMap_->GetID()
    );
    sceneResources_.registry.setTexture(
        sceneResources_.lightingHandles.prefilterMap,
        prefilterMap_->GetID()
    );

    applyExtractedSun(hdrTexture_->getExtractedSun());
    return true;
}

void EnvironmentController::applyExtractedSun(const ExtractedLight& extractedSun)
{
     
    const glm::vec3 sunSourceDirection = extractedSun.valid
        ? extractedSun.direction
        : extractedSun.brightestDirection;

    lightSettings_.environmentSunDirection = sunSourceDirection;
    lightSettings_.sunDirection = -sunSourceDirection;
    lightSettings_.sunExtractedFromEnvironment = extractedSun.valid;
}

void EnvironmentController::applyPreset()
{
    sceneConfig_.renderMode = RenderMode::Lighting;
    sceneConfig_.forwardLightMode = ForwardLightMode::Light;
    sceneConfig_.enableSkybox = true;
    sceneConfig_.enableGammaCorrection = true;
    sceneConfig_.enableHDR = true;
    sceneConfig_.enableBloom = true;
    sceneConfig_.enableSSAO = true;
    sceneConfig_.enablePBR = true;
    sceneConfig_.enableIBL = true;
    sceneConfig_.enableGI = true;
    sceneConfig_.phongDiffuseStrength = 0.55f;
    sceneConfig_.phongSpecularStrength = 0.18f;
    sceneConfig_.phongIBLDiffuseStrength = 1.25f;
    sceneConfig_.phongIBLSpecularStrength = 0.35f;
    renderParams_.enableNormalMapping = true;
    renderParams_.enableParallaxMapping = false;
    renderParams_.bumpNormalStrength = 1.0f;
    renderParams_.numLayers = 32;
    renderParams_.parallaxHeightScale = 0.006f;

    // The procedural sky intentionally uses a flat color, so keep it visually
    // paired with the selected HDR environment instead of leaving the previous
    // environment's color behind after a cubemap switch.
    switch (sceneConfig_.environmentSelection)
    {
    case EnvironmentSelection::Sunny:
        sceneConfig_.skyTopColor = glm::vec3(0.24f, 0.55f, 0.90f);
        break;
    case EnvironmentSelection::GodRays:
        sceneConfig_.skyTopColor = glm::vec3(0.34f, 0.42f, 0.54f);
        break;
    case EnvironmentSelection::NightN8_3K:
        sceneConfig_.skyTopColor = glm::vec3(0.018f, 0.030f, 0.070f);
        break;
    case EnvironmentSelection::Night:
    default:
        sceneConfig_.skyTopColor = glm::vec3(0.030f, 0.018f, 0.045f);
        break;
    }

    switch (sceneConfig_.environmentSelection)
    {
    case EnvironmentSelection::Sunny:
    case EnvironmentSelection::GodRays:
        sceneConfig_.enablePointLight = false;
        sceneConfig_.enableDirectionalLight = true;
        sceneConfig_.enableFlashlight = false;
        sceneConfig_.fixedAmbientColor = glm::vec3(46.0f, 14.0f, 6.0f) / 255.0f;
        sceneConfig_.fixedAmbientStrength = 0.15f;
        sceneConfig_.iblAmbientTint = glm::vec3(1.0f);
        sceneConfig_.iblAmbientStrength = 0.15f;
        sceneConfig_.phongDiffuseStrength = 0.42f;
        sceneConfig_.phongSpecularStrength = 0.10f;
        sceneConfig_.phongIBLDiffuseStrength = 1.15f;
        sceneConfig_.phongIBLSpecularStrength = 0.26f;
        sceneConfig_.enableGI = true;
        sceneConfig_.giStrength = 2.12f;
        sceneConfig_.giRadius = 18.2f;
        sceneConfig_.giMaxDistance = 12.1f;
        sceneConfig_.giSampleCount = 7;
        sceneConfig_.ssaoStrength = 3.11f;
        sceneConfig_.exposure = 0.5f;
        sceneConfig_.bloomStrength = 0.24f;
        sceneConfig_.bloomThreshold = 0.46f;
        sceneConfig_.numBlurPasses = 10;
        renderParams_.enableParallaxMapping = true;
        renderParams_.parallaxHeightScale = 0.006f;
        renderParams_.bumpNormalStrength = 1.0f;
        renderParams_.numLayers = 32;
        lightSettings_.sunDiffuse = glm::vec3(38.0f, 31.0f, 15.0f);
        lightSettings_.sunSpecular = lightSettings_.sunDiffuse;
        lightSettings_.sunAmbient = glm::vec3(1.0f, 1.5f, 2.5f);
        lightSettings_.sunIntensity = 0.7f;
        lightSettings_.sunIntensityScale = 0.76f;
        lightSettings_.sunShadowStrength = 1.0f;
        sceneConfig_.directionalShadowLightSize = 0.004f;
        sceneConfig_.directionalShadowBlockerSearchRadius = 0.006f;
        sceneConfig_.directionalShadowMinFilterRadius = 0.001f;
        sceneConfig_.directionalShadowMaxFilterRadius = 0.005f;
        sceneConfig_.directionalShadowBiasSlope = 0.005f;
        sceneConfig_.directionalShadowBiasMin = 0.0005f;
        break;

    case EnvironmentSelection::NightN8_3K:
        sceneConfig_.enablePointLight = false;
        sceneConfig_.enableDirectionalLight = true;
        sceneConfig_.enableFlashlight = false;
        sceneConfig_.fixedAmbientStrength = 0.0f;
        sceneConfig_.iblAmbientTint = glm::vec3(1.0f, 0.6f, 0.9f);
        sceneConfig_.iblAmbientStrength = 2.8f;
        sceneConfig_.phongDiffuseStrength = 0.48f;
        sceneConfig_.phongSpecularStrength = 0.12f;
        sceneConfig_.phongIBLDiffuseStrength = 0.95f;
        sceneConfig_.phongIBLSpecularStrength = 0.24f;
        sceneConfig_.ssaoStrength = 1.8f;
        sceneConfig_.exposure = 0.8f;
        sceneConfig_.bloomStrength = 2.2f;
        sceneConfig_.bloomThreshold = 0.9f;
        renderParams_.numLayers = 48;
        lightSettings_.sunDiffuse = glm::vec3(2.0f, 14.0f, 25.0f);
        lightSettings_.sunSpecular = lightSettings_.sunDiffuse;
        lightSettings_.sunAmbient = glm::vec3(0.5f, 0.0f, 0.8f);
        lightSettings_.sunIntensity = 0.45f;
        lightSettings_.sunIntensityScale = 0.5f;
        lightSettings_.sunShadowStrength = 0.81f;
        sceneConfig_.directionalShadowLightSize = 0.015f;
        sceneConfig_.directionalShadowBlockerSearchRadius = 0.015f;
        sceneConfig_.directionalShadowMinFilterRadius = 0.002f;
        sceneConfig_.directionalShadowMaxFilterRadius = 0.012f;
        break;

    case EnvironmentSelection::Night:
    default:
        // Warm after-sunset key light: the sun sits just below the horizon,
        // producing long soft shadows while the night HDR keeps the sky dark.
        sceneConfig_.enablePointLight = false;
        sceneConfig_.enableDirectionalLight = true;
        sceneConfig_.enableFlashlight = false;
        sceneConfig_.fixedAmbientColor = glm::vec3(0.18f, 0.055f, 0.025f);
        sceneConfig_.fixedAmbientStrength = 0.18f;
        sceneConfig_.iblAmbientTint = glm::vec3(1.0f, 0.62f, 0.42f);
        sceneConfig_.iblAmbientStrength = 0.42f;
        sceneConfig_.phongDiffuseStrength = 0.62f;
        sceneConfig_.phongSpecularStrength = 0.14f;
        sceneConfig_.phongIBLDiffuseStrength = 1.35f;
        sceneConfig_.phongIBLSpecularStrength = 0.30f;
        sceneConfig_.ssaoStrength = 1.9f;
        sceneConfig_.exposure = 1.15f;
        sceneConfig_.bloomStrength = 1.15f;
        sceneConfig_.bloomThreshold = 1.0f;
        lightSettings_.sunDirection = glm::normalize(glm::vec3(-0.82f, -0.16f, -0.55f));
        lightSettings_.sunDiffuse = glm::vec3(24.0f, 7.5f, 1.8f);
        lightSettings_.sunSpecular = glm::vec3(18.0f, 6.0f, 2.0f);
        lightSettings_.sunAmbient = glm::vec3(0.42f, 0.12f, 0.055f);
        lightSettings_.sunIntensity = 0.62f;
        lightSettings_.sunIntensityScale = 0.58f;
        lightSettings_.sunShadowStrength = 0.92f;
        lightSettings_.sunExtractedFromEnvironment = false;
        sceneConfig_.directionalShadowLightSize = 0.012f;
        sceneConfig_.directionalShadowBlockerSearchRadius = 0.014f;
        sceneConfig_.directionalShadowMinFilterRadius = 0.0015f;
        sceneConfig_.directionalShadowMaxFilterRadius = 0.014f;
        lightSettings_.flashDiffuse = glm::vec3(0.8f, 0.9f, 1.0f);
        lightSettings_.flashSpecular = glm::vec3(1.0f);
        lightSettings_.flashIntensity = 12.0f;
        lightSettings_.flashShadowStrength = 0.9f;
        lightSettings_.flashConstant = 1.0f;
        lightSettings_.flashLinear = 0.045f;
        lightSettings_.flashQuadratic = 0.0075f;
        lightSettings_.flashCutOff = 0.96f;
        lightSettings_.flashOuterCutOff = 0.91f;
        break;
    }
}
