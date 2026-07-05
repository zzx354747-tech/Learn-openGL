#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/resources/framebuffer/Gbuffer.h"

class SceneObjectPass
{
public:
    SceneObjectPass(
        SceneRenderResources& resources,
        SceneRenderConfig&    config,
        Camera&               camera,
        SphereDrawer&         sphereDrawer,
        GBuffer&              gBuffer,
        RenderParams&         renderParams);

    void render(int bfwidth, int bfheight);

private:
    SceneRenderResources& resources;
    SceneRenderConfig&    config;
    Camera&               camera;
    SphereDrawer&         sphereDrawer;
    GBuffer&              gBuffer;
    RenderParams&         renderParams;

    void renderSpheres(int bfwidth, int bfheight);
    Shader* getGeometryShader();
};
