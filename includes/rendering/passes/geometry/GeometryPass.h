#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/resources/framebuffer/Gbuffer.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/RenderParams.h"

class GeometryPass
{
public:
    GeometryPass(
        SceneRenderResources& resources,
        SceneRenderConfig&    config,
        Camera&               camera,
        SphereDrawer&         sphereDrawer,
        ModelDrawer&          modelDrawer,
        GBuffer&              gBuffer,
        RenderParams&         renderParams);

    void render(int bfwidth, int bfheight);

private:
    SceneRenderResources& resources;
    SceneRenderConfig&    config;
    Camera&               camera;
    SphereDrawer&         sphereDrawer;
    ModelDrawer&          modelDrawer;
    GBuffer&              gBuffer;
    RenderParams&         renderParams;

    void renderSpheres(int bfwidth, int bfheight);
    void renderModels(int bfwidth, int bfheight);

    Shader* getPBRShader();
};