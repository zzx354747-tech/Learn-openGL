#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources, SceneRenderResources& resources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, DirectionalShadowPass& directionalShadowPass, PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass, GeometryPass& geometryPass, LightingPass& lightingPass, GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass) : config(config), resources(resources), state(state), directionalShadowPass(directionalShadowPass), pointShadowPass(pointShadowPass), spotShadowPass(spotShadowPass), shadowDebugPass(resources, shadowResources), deferredRenderPass(config, geometryPass, ssaoCommonPass, gBuffer, lightingPass), forwardHDRPass(camera, resources, config, state), forwardOverlayPass(camera, resources, state, config), screenPass(config, resources)
{
        (void)lightSettings;
    }

void SceneRender::render( int bfwidth, int bfheight, Shader& screenShader, Screenquad& screenQuad, Framebuffer& framebuffer)
{
        directionalShadowPass.render();
        pointShadowPass.render(state.lightPositions);
        spotShadowPass.render();

        if (config.renderMode == RenderMode::ShadowDebug && resources.shadowDebugShader)
        {
            shadowDebugPass.render(bfwidth, bfheight, screenQuad);
            return;
        }

        if (config.renderMode == RenderMode::Lighting)
        {
            deferredRenderPass.render(bfwidth, bfheight, framebuffer, screenQuad);
            forwardOverlayPass.render(bfwidth, bfheight, framebuffer);
        }
        else
        {
            forwardHDRPass.render(bfwidth, bfheight, framebuffer);
        }

        if (config.enableBloom && resources.pingpongFBO && resources.blurShader)
        {
            blurPass.render(
                framebuffer,
                *resources.pingpongFBO,
                *resources.blurShader,
                screenQuad,
                config.numBlurPasses);
        }

        screenPass.render(bfwidth, bfheight, screenShader, screenQuad, framebuffer);
    }
