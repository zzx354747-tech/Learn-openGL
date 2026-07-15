#pragma once

#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/assets/light/LightSettings.h"

class SkyboxPass
{
public:
    static void renderSkyboxPass( 
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        LightSettings& lightSettings,
        int bfwidth, 
        int bfheight);

private:
    static void bindSkyboxTexture(EnvCubemap* skybox);
};
