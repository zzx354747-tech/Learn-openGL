#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources,
    SceneRenderResources& resources, SceneRenderConfig& config,
    SceneRenderState& state, LightSettings& lightSettings,
    DirectionalShadowPass& directionalShadowPass, BrightPrefilterPass& brightPrefilterPass,
    DownSamplePass& downSamplePassL1,
    PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass,
    GeometryPass& geometryPass, LightingPass& lightingPass,
    GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass,
    SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer)
    : config(config)
    , resources(resources)
    , state(state)
    , brightPrefilterPass(brightPrefilterPass)
    , downSamplePassL1(downSamplePassL1)
    , directionalShadowPass(directionalShadowPass)
    , pointShadowPass(pointShadowPass)
    , spotShadowPass(spotShadowPass)
    , shadowDebugPass(resources, shadowResources)
    , deferredRenderPass(config, geometryPass, ssaoCommonPass, gBuffer, lightingPass)
    , forwardHDRPass(camera, resources, config, state, sphereDrawer, modelDrawer)
    , forwardOverlayPass(camera, resources, state, config)
    , screenPass(config, resources)
    {
        (void)lightSettings;
    }

void SceneRender::render( int bfwidth, int bfheight, Shader& screenShader, Screenquad& screenQuad, Framebuffer& framebuffer)
{
        directionalShadowPass.render();
        pointShadowPass.render(state.lightPositions);
        spotShadowPass.render();

        if (config.renderMode == RenderMode::ShadowDebug && resources.shaderLibrary)
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

        if (config.enableBloom && resources.shaderLibrary)
        {
            brightPrefilterPass.setThreshold(config.bloomThreshold);
            brightPrefilterPass.render(framebuffer.getTextureID(0));

            downSamplePassL1.render(
                brightPrefilterPass.GetBrightPrefilterTextureID());
        }

        screenPass.render(bfwidth, bfheight, screenShader, screenQuad, framebuffer);
    }
