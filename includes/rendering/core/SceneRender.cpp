#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources, SceneRenderResources& resources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, DirectionalShadowPass& directionalShadowPass, PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass, GeometryPass& geometryPass, LightingPass& lightingPass, GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass, SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer, int width, int height) : config(config), resources(resources), state(state), directionalShadowPass(directionalShadowPass), pointShadowPass(pointShadowPass), spotShadowPass(spotShadowPass), shadowDebugPass(resources, shadowResources), deferredRenderPass(config, geometryPass, ssaoCommonPass, gBuffer, lightingPass), forwardHDRPass(camera, resources, config, state, lightSettings, sphereDrawer, modelDrawer), forwardOverlayPass(camera, resources, state, config, lightSettings), screenPass(config, resources, camera, lightSettings), temporalAAPass(width, height, camera, config)
{
        (void)lightSettings;
    }

void SceneRender::render( int bfwidth, int bfheight, Shader& screenShader, Screenquad& screenQuad, Framebuffer& framebuffer)
{
        temporalAAPass.beginFrame(bfwidth, bfheight);
        // Dense procedural vegetation is expensive in shadow maps. Only build
        // maps for lights that can actually contribute to the current frame.
        const bool lightingMode = config.renderMode == RenderMode::Lighting;
        if (lightingMode && config.enableDirectionalLight)
            directionalShadowPass.render();
        if (lightingMode && config.enablePointLight)
            pointShadowPass.render(state.lightPositions);
        if (lightingMode && config.enableFlashlight)
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

        if (config.enableBloom && resources.pingpongFBO && resources.shaderLibrary)
        {
            blurPass.render(
                framebuffer,
                *resources.pingpongFBO,
                resources.shaderLibrary->blur,
                screenQuad,
                config.numBlurPasses);
        }

        GLuint sceneTexture = framebuffer.getTextureID(0);
        if (config.enableTAA && resources.shaderLibrary)
        {
            sceneTexture = temporalAAPass.resolve(
                bfwidth,
                bfheight,
                framebuffer,
                screenQuad,
                resources.shaderLibrary->taa);
        }

        screenPass.render(
            bfwidth,
            bfheight,
            screenShader,
            screenQuad,
            framebuffer,
            sceneTexture);
    }

void SceneRender::resize(int width, int height)
{
    temporalAAPass.resize(width, height);
}
