#include "rendering/passes/forward/ForwardOverlayPass.h"

void ForwardOverlayPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer
)
{
    framebuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glEnable(GL_DEPTH_TEST);

    lightVisualPass::renderLightVisualPass(
        camera,
        resources,
        state,
        config,
        bfwidth,
        bfheight
    );

    SkyboxPass::renderSkyboxPass(
        camera,
        resources,
        config,
        bfwidth,
        bfheight
    );

    framebuffer.unbind();
}

ForwardOverlayPass::ForwardOverlayPass( Camera& camera, SceneRenderResources& resources, SceneRenderState& state, SceneRenderConfig& config ) : camera(camera), resources(resources), state(state), config(config)
{
    }
