// rendering/core/RendererScene.cpp
#include "rendering/core/RendererScene.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
double weatherClockSeconds()
{
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

unsigned int nextStormHoleSeed()
{
    static std::uint32_t state = static_cast<std::uint32_t>(
        std::chrono::high_resolution_clock::now()
            .time_since_epoch().count());
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    // GLSL receives this as an int and converts it to float for hashing. Keep
    // it small enough to preserve every bit in a 32-bit shader float.
    return 1u + state % 65520u;
}

int stormHoleCountFromSeed(unsigned int seed)
{
    return 3 + static_cast<int>((seed >> 3u) % 5u);
}

SceneRenderConfig makeCloudWeatherTarget(
    const SceneRenderConfig& current,
    CloudWeatherPreset preset)
{
    SceneRenderConfig target = current;
    target.enableProceduralSky = true;
    target.enableVolumetricClouds = true;
    target.enableGodRays = true;

    switch (preset)
    {
        case CloudWeatherPreset::Storm:
            // Neutral slate-gray background blends into the dense storm base
            // without the previous saturated blue seam at cloud openings.
            target.skyTopColor = glm::vec3(0.075f, 0.085f, 0.105f);
            target.cloudCoverage = 0.94f;
            target.cloudDensity = 2.15f;
            target.cloudBaseHeight = 4000.0f;
            target.cloudThickness = 2400.0f;
            target.cloudScale = 1.35f;
            target.cloudDetailScale = 4.50f;
            target.cloudType = 0.82f;
            target.cloudAnvilAmount = 0.88f;
            target.cloudErosionStrength = 0.34f;
            target.stormHoleStrength = 1.0f;
            target.stormHoleMinRadius = 160.0f;
            target.stormHoleMaxRadius = 1400.0f;
            target.stormHoleSoftness = 0.38f;
            target.stormHoleShaftStrength = 2.0f;
            target.stormShaftLean = glm::vec2(0.08f, 0.0f);
            target.cloudSpeed = 24.0f;
            target.cloudEvolutionSpeed = 0.15f;
            target.cloudWindDirection = glm::vec2(0.85f, 0.53f);
            target.cloudWindShear = 0.58f;
            target.cloudExtinction = 2.35f;
            target.cloudLightAbsorption = 2.65f;
            target.cloudAmbientStrength = 0.18f;
            target.cloudPowderStrength = 2.10f;
            target.cloudMultiScattering = 0.16f;
            target.cloudSilverLining = 2.20f;
            target.cloudForwardScattering = 0.72f;
            target.cloudBackwardScattering = -0.32f;
            target.cloudSunColor = glm::vec3(0.72f, 0.76f, 0.84f);
            target.cloudBottomColor = glm::vec3(0.045f, 0.060f, 0.10f);
            target.cloudTopColor = glm::vec3(0.30f, 0.34f, 0.42f);
            target.cloudViewSteps = 56;
            target.cloudLightSteps = 6;
            target.godRayIntensity = 1.35f;
            target.godRayDensity = 1.18f;
            target.godRayDecay = 0.980f;
            target.godRayWeight = 0.075f;
            target.godRayExposure = 0.95f;
            target.godRayRadius = 0.78f;
            target.godRaySamples = 64;
            target.godRayColor = glm::vec3(1.0f, 0.80f, 0.54f);
            break;

        case CloudWeatherPreset::Overcast:
            target.skyTopColor = glm::vec3(0.42f, 0.47f, 0.54f);
            target.cloudCoverage = 0.86f;
            target.cloudDensity = 1.55f;
            target.cloudBaseHeight = 850.0f;
            target.cloudThickness = 1550.0f;
            target.cloudScale = 1.08f;
            target.cloudDetailScale = 3.00f;
            target.cloudType = 0.32f;
            target.cloudAnvilAmount = 0.12f;
            target.cloudErosionStrength = 0.12f;
            target.stormHoleStrength = 0.0f;
            target.cloudSpeed = 10.0f;
            target.cloudEvolutionSpeed = 0.018f;
            target.cloudWindDirection = glm::vec2(0.90f, 0.43f);
            target.cloudWindShear = 0.18f;
            target.cloudExtinction = 1.75f;
            target.cloudLightAbsorption = 1.90f;
            target.cloudAmbientStrength = 0.40f;
            target.cloudPowderStrength = 1.20f;
            target.cloudMultiScattering = 0.30f;
            target.cloudSilverLining = 0.35f;
            target.cloudForwardScattering = 0.55f;
            target.cloudBackwardScattering = -0.28f;
            target.cloudSunColor = glm::vec3(0.75f, 0.79f, 0.84f);
            target.cloudBottomColor = glm::vec3(0.13f, 0.16f, 0.21f);
            target.cloudTopColor = glm::vec3(0.48f, 0.53f, 0.60f);
            target.cloudViewSteps = 48;
            target.cloudLightSteps = 5;
            target.godRayIntensity = 0.18f;
            target.godRayDensity = 0.86f;
            target.godRayDecay = 0.958f;
            target.godRayWeight = 0.040f;
            target.godRayExposure = 0.45f;
            target.godRayRadius = 0.34f;
            target.godRaySamples = 40;
            target.godRayColor = glm::vec3(0.78f, 0.82f, 0.88f);
            break;

        case CloudWeatherPreset::Sunny:
        default:
            target.skyTopColor = glm::vec3(0.24f, 0.55f, 0.90f);
            target.cloudCoverage = 0.32f;
            target.cloudDensity = 0.92f;
            target.cloudBaseHeight = 1500.0f;
            target.cloudThickness = 1050.0f;
            target.cloudScale = 0.82f;
            target.cloudDetailScale = 3.80f;
            target.cloudType = 0.78f;
            target.cloudAnvilAmount = 0.18f;
            target.cloudErosionStrength = 0.28f;
            target.stormHoleStrength = 0.0f;
            target.cloudSpeed = 7.0f;
            target.cloudEvolutionSpeed = 0.035f;
            target.cloudWindDirection = glm::vec2(0.94f, 0.34f);
            target.cloudWindShear = 0.08f;
            target.cloudExtinction = 1.05f;
            target.cloudLightAbsorption = 0.90f;
            target.cloudAmbientStrength = 0.72f;
            target.cloudPowderStrength = 1.35f;
            target.cloudMultiScattering = 0.48f;
            target.cloudSilverLining = 1.85f;
            target.cloudForwardScattering = 0.68f;
            target.cloudBackwardScattering = -0.20f;
            target.cloudSunColor = glm::vec3(1.0f, 0.91f, 0.70f);
            target.cloudBottomColor = glm::vec3(0.34f, 0.46f, 0.63f);
            target.cloudTopColor = glm::vec3(0.84f, 0.91f, 1.0f);
            target.cloudViewSteps = 48;
            target.cloudLightSteps = 5;
            target.godRayIntensity = 0.82f;
            target.godRayDensity = 0.90f;
            target.godRayDecay = 0.967f;
            target.godRayWeight = 0.058f;
            target.godRayExposure = 0.82f;
            target.godRayRadius = 0.38f;
            target.godRaySamples = 48;
            target.godRayColor = glm::vec3(1.0f, 0.78f, 0.50f);
            break;
    }

    return target;
}

float blendFloat(float start, float target, float amount)
{
    return start + (target - start) * amount;
}

void blendCloudWeather(
    SceneRenderConfig& current,
    const SceneRenderConfig& start,
    const SceneRenderConfig& target,
    float amount)
{
    current.skyTopColor = glm::mix(start.skyTopColor, target.skyTopColor, amount);
    current.cloudCoverage = blendFloat(start.cloudCoverage, target.cloudCoverage, amount);
    current.cloudDensity = blendFloat(start.cloudDensity, target.cloudDensity, amount);
    current.cloudBaseHeight = blendFloat(start.cloudBaseHeight, target.cloudBaseHeight, amount);
    current.cloudThickness = blendFloat(start.cloudThickness, target.cloudThickness, amount);
    current.cloudScale = blendFloat(start.cloudScale, target.cloudScale, amount);
    current.cloudDetailScale = blendFloat(start.cloudDetailScale, target.cloudDetailScale, amount);
    current.cloudType = blendFloat(start.cloudType, target.cloudType, amount);
    current.cloudAnvilAmount = blendFloat(start.cloudAnvilAmount, target.cloudAnvilAmount, amount);
    current.cloudErosionStrength = blendFloat(start.cloudErosionStrength, target.cloudErosionStrength, amount);
    current.stormHoleStrength = blendFloat(start.stormHoleStrength, target.stormHoleStrength, amount);
    current.stormHoleMinRadius = blendFloat(
        start.stormHoleMinRadius, target.stormHoleMinRadius, amount);
    current.stormHoleMaxRadius = blendFloat(
        start.stormHoleMaxRadius, target.stormHoleMaxRadius, amount);
    current.stormHoleSoftness = blendFloat(start.stormHoleSoftness, target.stormHoleSoftness, amount);
    current.stormHoleShaftStrength = blendFloat(start.stormHoleShaftStrength, target.stormHoleShaftStrength, amount);
    current.stormShaftLean = glm::mix(
        start.stormShaftLean, target.stormShaftLean, amount);
    current.cloudSpeed = blendFloat(start.cloudSpeed, target.cloudSpeed, amount);
    current.cloudEvolutionSpeed = blendFloat(start.cloudEvolutionSpeed, target.cloudEvolutionSpeed, amount);
    current.cloudWindDirection = glm::mix(start.cloudWindDirection, target.cloudWindDirection, amount);
    current.cloudWindShear = blendFloat(start.cloudWindShear, target.cloudWindShear, amount);
    current.cloudExtinction = blendFloat(start.cloudExtinction, target.cloudExtinction, amount);
    current.cloudLightAbsorption = blendFloat(start.cloudLightAbsorption, target.cloudLightAbsorption, amount);
    current.cloudAmbientStrength = blendFloat(start.cloudAmbientStrength, target.cloudAmbientStrength, amount);
    current.cloudPowderStrength = blendFloat(start.cloudPowderStrength, target.cloudPowderStrength, amount);
    current.cloudMultiScattering = blendFloat(start.cloudMultiScattering, target.cloudMultiScattering, amount);
    current.cloudSilverLining = blendFloat(start.cloudSilverLining, target.cloudSilverLining, amount);
    current.cloudForwardScattering = blendFloat(start.cloudForwardScattering, target.cloudForwardScattering, amount);
    current.cloudBackwardScattering = blendFloat(start.cloudBackwardScattering, target.cloudBackwardScattering, amount);
    current.cloudSunColor = glm::mix(start.cloudSunColor, target.cloudSunColor, amount);
    current.cloudBottomColor = glm::mix(start.cloudBottomColor, target.cloudBottomColor, amount);
    current.cloudTopColor = glm::mix(start.cloudTopColor, target.cloudTopColor, amount);
    current.cloudViewSteps = static_cast<int>(std::round(blendFloat(
        static_cast<float>(start.cloudViewSteps),
        static_cast<float>(target.cloudViewSteps), amount)));
    current.cloudLightSteps = static_cast<int>(std::round(blendFloat(
        static_cast<float>(start.cloudLightSteps),
        static_cast<float>(target.cloudLightSteps), amount)));
    current.godRayIntensity = blendFloat(start.godRayIntensity, target.godRayIntensity, amount);
    current.godRayDensity = blendFloat(start.godRayDensity, target.godRayDensity, amount);
    current.godRayDecay = blendFloat(start.godRayDecay, target.godRayDecay, amount);
    current.godRayWeight = blendFloat(start.godRayWeight, target.godRayWeight, amount);
    current.godRayExposure = blendFloat(start.godRayExposure, target.godRayExposure, amount);
    current.godRayRadius = blendFloat(start.godRayRadius, target.godRayRadius, amount);
    current.godRaySamples = static_cast<int>(std::round(blendFloat(
        static_cast<float>(start.godRaySamples),
        static_cast<float>(target.godRaySamples), amount)));
    current.godRayColor = glm::mix(start.godRayColor, target.godRayColor, amount);
}
}

RendererScene::RendererScene(int width, int height)
    : fb(width, height)
    , pingpongFBO(width, height)
    , sceneGBuffer(width, height)
    , sceneSSAO(width, height)
    , vegetationMesh(terrainMesh)
    , shadowMap(4096, 4096)
    , pointShadowMap(1024, 1024, 1.0f, 50.0f)
    , spotShadowMap(1024, 1024, 1.0f, 50.0f)
    , sceneConfig(makeLivingRoomPreset())
    , sphereDrawer(&sphereMesh, &sceneState, &sceneConfig)
    , livingRoomDrawer("../3D_model/living_room_interior_free.glb", &sceneConfig)
    , pointShadowPass(pointShadowMap,
                      shaderLibrary.pointShadowMap,
                      sphereDrawer,
                      livingRoomDrawer,
                      sceneResources.registry,
                      sceneResources.lightingHandles.depthCubeMap)
    , directionalShadowPass(shadowMap,
                            shaderLibrary.shadowMap,
                            sphereDrawer,
                            livingRoomDrawer,
                            sceneState,
                            lightSettings,
                            sceneConfig,
                            sceneResources.registry,
                            sceneResources.lightingHandles.shadowMap)
    , spotShadowPass(spotShadowMap,
                      shaderLibrary.shadowMap,
                      sphereDrawer,
                      livingRoomDrawer,
                      camera,
                      sceneState,
                      lightSettings,
                      sceneResources.registry,
                      sceneResources.lightingHandles.spotShadowMap)
    , brdfLUT(shaderLibrary.brdf)
    , environmentController(sceneConfig,
                             renderParams,
                             lightSettings,
                             sceneResources,
                             shaderLibrary.irradiance,
                             shaderLibrary.prefilter)
    , geometryPass(sceneResources,
                   sceneConfig,
                   camera,
                   sphereDrawer,
                   livingRoomDrawer,
                   sceneGBuffer,
                   renderParams)
    , lightingPass(sceneResources,
                   shadowResources,
                   sceneConfig,
                   sceneState,
                   lightSettings,
                   camera)
    , ssaoCommonPass(sceneResources,
                      sceneSSAO,
                      screenQuad,
                      camera,
                      sceneGBuffer)
    , sceneRender(camera,
                  shadowResources,
                  sceneResources,
                  sceneConfig,
                  sceneState,
                  lightSettings,
                  directionalShadowPass,
                  pointShadowPass,
                  spotShadowPass,
                  geometryPass,
                  lightingPass,
                  sceneGBuffer,
                  ssaoCommonPass,
                  sphereDrawer,
                  livingRoomDrawer,
                  width,
                  height)
    , renderModeIndex(1)
    , environmentIndex(getEnvironmentIndex(sceneConfig.environmentSelection))
    , uiState{
          renderModeIndex,
          environmentIndex,
          sceneConfig,
          renderParams,
          lightSettings,
          false,
          [this]() { return environmentController.load(); },
          [this]() { environmentController.applyPreset(); }
      }
{
    sceneResources.shaderLibrary = &shaderLibrary;
    sceneResources.sphereMesh = &sphereMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;
    sceneResources.waterMesh = &waterMesh;

    sunTexture = std::make_unique<GLTexture>("../textures/sky/sun.png");
    sceneResources.sunTexture = sunTexture->getID();
    blueNoiseTexture = std::make_unique<GLTexture>("../textures/noise/blue_noise_64.png");
    sceneResources.blueNoiseTexture = blueNoiseTexture->getID();

    sphereDrawer.loadMaterials("../textures/PBR/");

    livingRoomDrawer.setVisibleInScene(SceneSelection::LivingRoom);
    livingRoomDrawer.setTerrainMesh(&terrainMesh);
    livingRoomDrawer.setVegetationMesh(&vegetationMesh);
    glm::mat4 livingRoomTransform(1.0f);
    livingRoomTransform = glm::translate(livingRoomTransform, glm::vec3(0.0f, 0.0f, -3.0f));
    livingRoomTransform = glm::scale(livingRoomTransform, glm::vec3(0.2f));
    livingRoomDrawer.setTransform(livingRoomTransform);

    shadowResources.shadowMap = &shadowMap;
    shadowResources.pointShadowMap = &pointShadowMap;
    shadowResources.spotShadowMap = &spotShadowMap;

    sceneResources.registry.setTexture(
        sceneResources.lightingHandles.brdfLUT,
        brdfLUT.GetID());

    sceneResources.brdfLUT = &brdfLUT;
    sceneResources.pingpongFBO = &pingpongFBO;

    const RenderMode startupRenderMode = sceneConfig.renderMode;
    const ForwardLightMode startupForwardLightMode =
        sceneConfig.forwardLightMode;
    environmentController.load();
    environmentController.applyPreset();
    // Environment presets intentionally switch interactive changes to the
    // lighting pipeline. Startup is different: preserve the scene preset's
    // requested unlit Living Room mode after the initial cubemap is prepared.
    sceneConfig.renderMode = startupRenderMode;
    sceneConfig.forwardLightMode = startupForwardLightMode;
}

void RendererScene::resize(int width, int height)
{
    fb.resize(width, height);
    pingpongFBO.resize(width, height);
    sceneGBuffer.resize(width, height);
    sceneSSAO.resize(width, height);
    sceneRender.resize(width, height);
}

void RendererScene::render(int width, int height)
{
    updateCloudWeatherEvolution();
    sceneRender.render(
        width,
        height,
        shaderLibrary.screen,
        screenQuad,
        fb
    );
}

void RendererScene::updateCloudWeatherEvolution()
{
    const double now = weatherClockSeconds();
    const float deltaTime = cloudWeatherLastUpdateSeconds > 0.0
        ? static_cast<float>(std::min(now - cloudWeatherLastUpdateSeconds, 0.10))
        : 0.0f;
    cloudWeatherLastUpdateSeconds = now;

    const float windLength = glm::length(sceneConfig.cloudWindDirection);
    const glm::vec2 windDirection = windLength > 0.0001f
        ? sceneConfig.cloudWindDirection / windLength
        : glm::vec2(1.0f, 0.0f);
    sceneConfig.cloudAnimationOffset +=
        windDirection * sceneConfig.cloudSpeed * deltaTime;
    sceneConfig.cloudEvolutionPhase +=
        std::max(sceneConfig.cloudEvolutionSpeed, 0.0f) * deltaTime;

    if (sceneConfig.cloudWeatherPreset != activeCloudWeatherPreset ||
        sceneConfig.cloudWeatherTransitionRequest != activeCloudWeatherTransitionRequest)
    {
        activeCloudWeatherPreset = sceneConfig.cloudWeatherPreset;
        activeCloudWeatherTransitionRequest = sceneConfig.cloudWeatherTransitionRequest;
        if (activeCloudWeatherPreset == CloudWeatherPreset::Storm)
        {
            sceneConfig.stormHoleSeed = nextStormHoleSeed();
            sceneConfig.stormHoleCount =
                stormHoleCountFromSeed(sceneConfig.stormHoleSeed);
        }
        cloudWeatherTransitionStart = sceneConfig;
        cloudWeatherTransitionElapsed = 0.0f;
        cloudWeatherTransitionActive = true;
        sceneConfig.cloudWeatherTransitionProgress = 0.0f;
        sceneConfig.enableProceduralSky = true;
        sceneConfig.enableVolumetricClouds = true;
        sceneConfig.enableGodRays = true;
    }

    if (!cloudWeatherTransitionActive)
        return;

    cloudWeatherTransitionElapsed += deltaTime;
    const float duration = std::max(sceneConfig.cloudWeatherTransitionDuration, 0.1f);
    const float linearProgress = std::min(cloudWeatherTransitionElapsed / duration, 1.0f);
    const float smoothProgress = linearProgress * linearProgress * (3.0f - 2.0f * linearProgress);
    const SceneRenderConfig target = makeCloudWeatherTarget(
        sceneConfig, activeCloudWeatherPreset);
    blendCloudWeather(sceneConfig, cloudWeatherTransitionStart, target, smoothProgress);
    sceneConfig.cloudWeatherTransitionProgress = linearProgress;

    if (linearProgress >= 1.0f)
        cloudWeatherTransitionActive = false;
}

void RendererScene::renderUI(float fps, float swapWaitMs)
{
    sceneRenderUI.renderUI(uiState, fps, swapWaitMs);
}

Camera& RendererScene::getCamera()
{
    return camera;
}
