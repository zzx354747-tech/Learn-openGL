#pragma once 

#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"

class lightVisualPass
{
public:
    static void renderLightVisualPass(
        Camera& camera, 
        SceneRenderResources1& resources, 
        SceneRenderState state,
        SceneRenderConfig& config,
        int bfwidth, 
        int bfheight);
};
