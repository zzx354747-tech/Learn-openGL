#pragma once

#include "scene/Camera.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"

class lightVisualPass
{
public:
    static void renderLightVisualPass(
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderState state,
        SceneRenderConfig& config,
        int bfwidth, 
        int bfheight);
};
