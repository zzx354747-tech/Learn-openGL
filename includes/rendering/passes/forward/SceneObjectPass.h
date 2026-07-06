#pragma once

#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/core/SphereDrawer.h"

class SceneObjectPass
{
public:
    static void renderSceneObjectPass(
        Camera&               camera,
        SceneRenderResources& resources,
        SceneRenderConfig&    config,
        SphereDrawer&         sphereDrawer,
        int                   bfwidth,
        int                   bfheight
    );
};