#include "rendering/passes/forward/ForwardHDRPass.h"
#include "rendering/passes/forward/SceneObjectPass.h"

ForwardHDRPass::ForwardHDRPass(
    Camera& camera,
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    SceneRenderState& state,
    LightSettings& lightSettings,
    SphereDrawer& sphereDrawer,
    ModelDrawer& modelDrawer)
    : camera(camera)
    , resources(resources)
    , config(config)
    , state(state)
    , lightSettings(lightSettings)
    , sphereDrawer(sphereDrawer)
    , modelDrawer(modelDrawer)
{}

void ForwardHDRPass::render(int bfwidth, int bfheight, Framebuffer& framebuffer)
{
    framebuffer.bind();
    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(config.skyTopColor.r, config.skyTopColor.g, config.skyTopColor.b, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SceneObjectPass::renderSceneObjectPass(
        camera,
        resources,
        config,
        sphereDrawer,
        modelDrawer,
        bfwidth,
        bfheight);

    lightVisualPass::renderLightVisualPass(camera, resources, state, config, bfwidth, bfheight);
    SkyboxPass::renderSkyboxPass(camera, resources, config, lightSettings, bfwidth, bfheight);

    framebuffer.unbind();
}
