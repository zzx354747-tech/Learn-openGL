#pragma once

#include <glad/gl.h>

#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"

#include "rendering/passes/forward/SceneScenePass.h"
#include "rendering/passes/debug/LightVisualPass.h"
#include "rendering/passes/environment/SkyboxPass.h"

class ForwardHDRPass
{
public:
    ForwardHDRPass( Camera& camera, SceneObjectPass& objectPass, SceneRenderResources& resources, SceneRenderConfig& config, SceneRenderState& state );

    void render(
        int bfwidth,
        int bfheight,
        Framebuffer& framebuffer
    );

private:
    Camera& camera;
    SceneObjectPass& objectPass;
    SceneRenderResources& resources;
    SceneRenderConfig& config;
    SceneRenderState& state;
};
