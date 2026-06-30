#pragma once

#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"

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
