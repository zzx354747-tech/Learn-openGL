#pragma once

#include <glad/gl.h>

#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/assets/mesh/Screenquad.h"

class ShadowDebugPass
{
public:
    ShadowDebugPass( SceneRenderResources& resources, ShadowResources& shadowResources );

    void render(int bfwidth, int bfheight, Screenquad& screenQuad);

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
};
