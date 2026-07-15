#pragma once

#include <glad/gl.h>

#include "core/Shader.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/assets/light/LightSettings.h"
#include "scene/Camera.h"

class ScreenPass
{
public:
    ScreenPass(SceneRenderConfig& config, SceneRenderResources& resources,
               Camera& camera, LightSettings& lightSettings);

    void render(
        int bfwidth,
        int bfheight,
        Shader& screenShader,
        Screenquad& screenQuad,
        Framebuffer& framebuffer,
        GLuint sceneTexture
    );

private:
    SceneRenderConfig& config;
    SceneRenderResources& resources;
    Camera& camera;
    LightSettings& lightSettings;
};
