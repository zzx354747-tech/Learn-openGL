// rendering/resources/scene/ScenePresets.cpp
#include "rendering/resources/scene/ScenePresets.h"

SceneRenderConfig makeLivingRoomPreset()
{
    SceneRenderConfig config;

    config.sceneSelection = SceneSelection::LivingRoom;
    config.renderMode = RenderMode::Lighting;
    config.forwardLightMode = ForwardLightMode::Light;

    config.enableSkybox = true;
    config.enablePointLight = true;
    config.enableDirectionalLight = false;
    config.enableFlashlight = false;
    config.enableGammaCorrection = false;
    config.enableBloom = false;
    config.enableSSAO = true;
    config.enablePBR = true;
    config.enableIBL = true;

    config.fixedAmbientColor = glm::vec3(0.08f);
    config.fixedAmbientStrength = 1.0f;
    config.iblAmbientTint = glm::vec3(1.0f);
    config.iblAmbientStrength = 1.0f;

    config.phongDiffuseStrength = 0.55f;
    config.phongSpecularStrength = 0.18f;
    config.phongIBLDiffuseStrength = 1.25f;
    config.phongIBLSpecularStrength = 0.35f;

    return config;
}