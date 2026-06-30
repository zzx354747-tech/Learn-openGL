#include "rendering/passes/forward/ForwardHDRPass.h"

void ForwardHDRPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer
)
{
    framebuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    objectPass.renderNoLightingPlane(bfwidth, bfheight);
    objectPass.renderNoLightingMaterialSpheres(bfwidth, bfheight);

    lightVisualPass::renderLightVisualPass(
        camera,
        resources,
        state,
        config,
        bfwidth,
        bfheight
    );

    if (resources.model)
    {
        objectPass.renderNoLightingModel(bfwidth, bfheight);
    }

    SkyboxPass::renderSkyboxPass(
        camera,
        resources,
        config,
        bfwidth,
        bfheight
    );

    framebuffer.unbind();
}

ForwardHDRPass::ForwardHDRPass( Camera& camera, SceneObjectPass& objectPass, SceneRenderResources& resources, SceneRenderConfig& config, SceneRenderState& state ) : camera(camera), objectPass(objectPass), resources(resources), config(config), state(state)
{
    }
