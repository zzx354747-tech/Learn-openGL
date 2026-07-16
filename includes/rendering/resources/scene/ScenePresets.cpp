// rendering/resources/scene/ScenePresets.cpp
#include "rendering/resources/scene/ScenePresets.h"

SceneRenderConfig makeLivingRoomPreset()
{
    SceneRenderConfig config;

    config.sceneSelection = SceneSelection::LivingRoom;
    config.environmentSelection = EnvironmentSelection::GodRays;
    config.renderMode = RenderMode::Basic;
    config.forwardLightMode = ForwardLightMode::Basic;

    config.enableSkybox = true;
    config.enablePointLight = false;
    config.enableDirectionalLight = true;
    config.enableFlashlight = false;
    config.enableGammaCorrection = true;
    config.enableHDR = true;
    config.enableBloom = true;
    config.enableSSAO = true;
    config.enablePBR = true;
    config.enableIBL = true;
    config.enableTAA = true;
    config.enableProceduralSky = true;
    config.enableVolumetricClouds = true;
    config.enableSunTexture = true;

    config.fixedAmbientColor = glm::vec3(46.0f, 14.0f, 6.0f) / 255.0f;
    config.fixedAmbientStrength = 0.15f;
    config.iblAmbientTint = glm::vec3(1.0f);
    config.iblAmbientStrength = 0.15f;

    config.phongDiffuseStrength = 0.42f;
    config.phongSpecularStrength = 0.10f;
    config.phongIBLDiffuseStrength = 1.15f;
    config.phongIBLSpecularStrength = 0.26f;

    config.enableGI = true;
    config.giStrength = 2.12f;
    config.giRadius = 18.2f;
    config.giMaxDistance = 12.1f;
    config.giSampleCount = 7;

    config.ssaoStrength = 3.11f;
    config.exposure = 0.5f;
    config.bloomStrength = 0.24f;
    config.bloomThreshold = 0.46f;
    config.numBlurPasses = 10;

    config.sunThreshold = 100.0f;
    config.directionalShadowLightSize = 0.004f;
    config.directionalShadowBlockerSearchRadius = 0.006f;
    config.directionalShadowMinFilterRadius = 0.001f;
    config.directionalShadowMaxFilterRadius = 0.005f;
    config.directionalShadowBiasSlope = 0.005f;
    config.directionalShadowBiasMin = 0.0005f;

    return config;
}
