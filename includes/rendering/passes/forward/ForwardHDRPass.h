#pragma once

#include <glad/gl.h>

#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/passes/debug/LightVisualPass.h"
#include "rendering/passes/environment/SkyboxPass.h"

class ForwardHDRPass
{
public:
    ForwardHDRPass(
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        SphereDrawer& sphereDrawer);

    void render(
        int bfwidth,
        int bfheight,
        Framebuffer& framebuffer
    );

private:
    Camera& camera;
    SceneRenderResources& resources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    SphereDrawer& sphereDrawer;
};
