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
        *sceneResources_.envCubemapShader);
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
    sceneConfig_.phongDiffuseStrength = 0.55f;
    sceneConfig_.phongSpecularStrength = 0.18f;
    sceneConfig_.phongIBLDiffuseStrength = 1.25f;
    sceneConfig_.phongIBLSpecularStrength = 0.35f;
    renderParams_.enableNormalMapping = true;
    renderParams_.enableParallaxMapping = false;
    renderParams_.bumpNormalStrength = 1.0f;
    renderParams_.numLayers = 32;
    renderParams_.parallaxHeightScale = 0.1f;

    switch (sceneConfig_.environmentSelection)
    {
    case EnvironmentSelection::Sunny:
        sceneConfig_.enablePointLight = false;
        sceneConfig_.enableDirectionalLight = true;
        sceneConfig_.enableFlashlight = false;
        sceneConfig_.fixedAmbientStrength = 0.1f;
        sceneConfig_.iblAmbientTint = glm::vec3(1.0f);
        sceneConfig_.iblAmbientStrength = 1.4f;
        sceneConfig_.phongDiffuseStrength = 0.42f;
        sceneConfig_.phongSpecularStrength = 0.10f;
        sceneConfig_.phongIBLDiffuseStrength = 1.15f;
        sceneConfig_.phongIBLSpecularStrength = 0.26f;
        sceneConfig_.ssaoStrength = 1.5f;
        sceneConfig_.exposure = 0.9f;
        sceneConfig_.bloomStrength = 0.6f;
        sceneConfig_.bloomThreshold = 1.3f;
        lightSettings_.sunDiffuse = glm::vec3(38.0f, 31.0f, 15.0f);
        lightSettings_.sunSpecular = lightSettings_.sunDiffuse;
        lightSettings_.sunAmbient = glm::vec3(1.0f, 1.5f, 2.5f);
        lightSettings_.sunIntensity = 0.7f;
        lightSettings_.sunIntensityScale = 0.52f;
        lightSettings_.sunShadowStrength = 0.94f;
        sceneConfig_.directionalShadowLightSize = 0.004f;
        sceneConfig_.directionalShadowBlockerSearchRadius = 0.006f;
        sceneConfig_.directionalShadowMinFilterRadius = 0.001f;
        sceneConfig_.directionalShadowMaxFilterRadius = 0.005f;
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
        sceneConfig_.enablePointLight = true;
        sceneConfig_.enableDirectionalLight = false;
        sceneConfig_.enableFlashlight = false;
        sceneConfig_.fixedAmbientStrength = 0.05f;
        sceneConfig_.iblAmbientTint = glm::vec3(1.0f);
        sceneConfig_.iblAmbientStrength = 0.15f;
        sceneConfig_.phongDiffuseStrength = 0.55f;
        sceneConfig_.phongSpecularStrength = 0.16f;
        sceneConfig_.phongIBLDiffuseStrength = 1.65f;
        sceneConfig_.phongIBLSpecularStrength = 0.42f;
        sceneConfig_.ssaoStrength = 2.5f;
        sceneConfig_.exposure = 1.4f;
        sceneConfig_.bloomStrength = 1.8f;
        sceneConfig_.bloomThreshold = 0.7f;
        lightSettings_.pointDiffuse = glm::vec3(1.0f, 0.5f, 0.1f);
        lightSettings_.pointSpecular = glm::vec3(1.0f, 0.6f, 0.25f);
        lightSettings_.pointIntensity = 15.0f;
        lightSettings_.pointAmbientIntensity = 0.2f;
        lightSettings_.pointShadowStrength = 0.98f;
        lightSettings_.pointConstant = 1.0f;
        lightSettings_.pointLinear = 0.09f;
        lightSettings_.pointQuadratic = 0.032f;
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
