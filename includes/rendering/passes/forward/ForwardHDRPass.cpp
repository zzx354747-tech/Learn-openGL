#include "rendering/passes/forward/ForwardHDRPass.h"
#include "rendering/passes/forward/SceneObjectPass.h"
#include "rendering/assets/mesh/AlpineVegetationSystem.h"

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

    const bool alpineVegetation =
        config.sceneSelection == SceneSelection::FujiTerrain &&
        config.enableVegetation && resources.vegetationSystem;
    const bool showcaseVegetation =
        config.sceneSelection == SceneSelection::Default &&
        config.enableVegetation && resources.vegetationSystem;
    if ((alpineVegetation || showcaseVegetation) &&
        resources.shaderLibrary)
    {
        // Basic/reflection modes still need the vegetation material and alpha
        // path. Only the lighting evaluation disappears in these modes.
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthRange(0.0, 1.0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        resources.shaderLibrary->vegetationUnlit.use();
        resources.shaderLibrary->vegetationUnlit.setFloat(
            "u_vegetationExposure", state.vegetationExposure);
        if (showcaseVegetation)
            resources.vegetationSystem->drawShowcase(
                resources.shaderLibrary->vegetationUnlit, camera, config);
        else
            resources.vegetationSystem->drawGeometry(
                resources.shaderLibrary->vegetationUnlit, camera, config);
    }

    lightVisualPass::renderLightVisualPass(camera, resources, state, config, bfwidth, bfheight);
    framebuffer.unbind();
}
