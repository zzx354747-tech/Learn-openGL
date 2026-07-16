#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources, SceneRenderResources& resources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, DirectionalShadowPass& directionalShadowPass, PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass, GeometryPass& geometryPass, LightingPass& lightingPass, GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass, SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer, int width, int height) : config(config), resources(resources), state(state), directionalShadowPass(directionalShadowPass), pointShadowPass(pointShadowPass), spotShadowPass(spotShadowPass), shadowDebugPass(resources, shadowResources), deferredRenderPass(config, geometryPass, ssaoCommonPass, gBuffer, lightingPass), forwardHDRPass(camera, resources, config, state, lightSettings, sphereDrawer, modelDrawer), forwardOverlayPass(camera, resources, state, config, lightSettings), volumetricSkyPass(camera, resources, config, lightSettings, width, height), volumetricLightPass(camera, shadowResources, resources, config, state, lightSettings, width, height), screenPass(config, resources, camera, lightSettings), temporalAAPass(width, height, camera, config)
{
        (void)lightSettings;
    }

void SceneRender::render( int bfwidth, int bfheight, Shader& screenShader, Screenquad& screenQuad, Framebuffer& framebuffer)
{
        gpuProfiler_.beginFrame();
        temporalAAPass.beginFrame(bfwidth, bfheight);
        // Cloud transmittance is consumed by deferred surface lighting, so it
        // must be current before the lighting pass begins.
        volumetricSkyPass.updateCloudAcceleration(screenQuad, gpuProfiler_);
        // Dense procedural vegetation is expensive in shadow maps. Only build
        // maps for lights that can actually contribute to the current frame.
        const bool lightingMode = config.renderMode == RenderMode::Lighting;
        if (lightingMode && config.enableDirectionalLight)
        {
            directionalShadowPass.render(screenQuad, gpuProfiler_);
        }
        if (lightingMode && config.enablePointLight)
        {
            ScopedGpuPass timer(gpuProfiler_, "Point Shadow");
            pointShadowPass.render(state.lightPositions);
        }
        if (lightingMode && config.enableFlashlight)
        {
            ScopedGpuPass timer(gpuProfiler_, "Spot Shadow");
            spotShadowPass.render();
        }

        if (config.renderMode == RenderMode::ShadowDebug && resources.shaderLibrary)
        {
            ScopedGpuPass timer(gpuProfiler_, "Shadow Debug");
            shadowDebugPass.render(bfwidth, bfheight, screenQuad);
            return;
        }

        if (config.renderMode == RenderMode::Lighting)
        {
            deferredRenderPass.render(
                bfwidth, bfheight, framebuffer, screenQuad, gpuProfiler_);
            {
                ScopedGpuPass timer(gpuProfiler_, "Forward Overlay");
                forwardOverlayPass.render(bfwidth, bfheight, framebuffer);
            }
        }
        else
        {
            ScopedGpuPass timer(gpuProfiler_, "Forward Scene");
            forwardHDRPass.render(bfwidth, bfheight, framebuffer);
        }

        volumetricSkyPass.render(
            bfwidth, bfheight, framebuffer, screenQuad, gpuProfiler_);

        volumetricLightPass.render(
            bfwidth,
            bfheight,
            framebuffer,
            screenQuad,
            volumetricSkyPass.cloudAccelerationPass(),
            gpuProfiler_);

        if (config.enableBloom && resources.pingpongFBO && resources.shaderLibrary)
        {
            ScopedGpuPass timer(gpuProfiler_, "Bloom Blur");
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
            ScopedGpuPass timer(gpuProfiler_, "TAA Resolve");
            sceneTexture = temporalAAPass.resolve(
                bfwidth,
                bfheight,
                framebuffer,
                screenQuad,
                resources.shaderLibrary->taa);
        }

        {
            ScopedGpuPass timer(gpuProfiler_, "Final Composite");
            screenPass.render(
                bfwidth,
                bfheight,
                screenShader,
                screenQuad,
                framebuffer,
                sceneTexture);
        }
    }

void SceneRender::resize(int width, int height)
{
    temporalAAPass.resize(width, height);
    volumetricSkyPass.resize(width, height);
    volumetricLightPass.resize(width, height);
}
