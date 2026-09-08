#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources,
    SceneRenderResources& resources, SceneRenderConfig& config,
    SceneRenderState& state, LightSettings& lightSettings,
    DirectionalShadowPass& directionalShadowPass, BrightPrefilterPass& brightPrefilterPass,
    DownSamplePass& downSamplePassL1, DownSamplePass& downSamplePassL2,
    DownSamplePass& downSamplePassL3, DownSamplePass& downSamplePassL4,
    DownSamplePass& downSamplePassL5,
    PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass,
    GeometryPass& geometryPass, LightingPass& lightingPass,
    GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass,
    SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer)
    : config(config)
    , resources(resources)
    , state(state)
    , brightPrefilterPass(brightPrefilterPass)
    , downSamplePassL1(downSamplePassL1)
    , downSamplePassL2(downSamplePassL2)
    , downSamplePassL3(downSamplePassL3)
    , downSamplePassL4(downSamplePassL4)
    , downSamplePassL5(downSamplePassL5)
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
            downSamplePassL2.render(
                downSamplePassL1.GetDownSampleTextureID());
            downSamplePassL3.render(
                downSamplePassL2.GetDownSampleTextureID());
            downSamplePassL4.render(
                downSamplePassL3.GetDownSampleTextureID());
            downSamplePassL5.render(
                downSamplePassL4.GetDownSampleTextureID());
        }

        screenPass.render(bfwidth, bfheight, screenShader, screenQuad, framebuffer);
    }
