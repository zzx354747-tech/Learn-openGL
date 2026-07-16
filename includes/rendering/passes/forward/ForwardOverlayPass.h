#pragma once

#include <glad/gl.h>

#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/light/LightSettings.h"

#include "rendering/passes/debug/LightVisualPass.h"
#include "rendering/passes/environment/SkyboxPass.h"

class ForwardOverlayPass
{
public:
    ForwardOverlayPass(Camera& camera, SceneRenderResources& resources,
        SceneRenderState& state, SceneRenderConfig& config, LightSettings& lightSettings);
    ~ForwardOverlayPass();

    void render(
        int bfwidth,
        int bfheight,
        Framebuffer& framebuffer
    );

private:
    Camera& camera;
    SceneRenderResources& resources;
    SceneRenderState& state;
    SceneRenderConfig& config;
    LightSettings& lightSettings;
    unsigned int sceneColorOpaque = 0;
    int opaqueWidth = 0;
    int opaqueHeight = 0;

    void renderWater(int bfwidth, int bfheight);
    void captureOpaqueScene(int width, int height);
};
