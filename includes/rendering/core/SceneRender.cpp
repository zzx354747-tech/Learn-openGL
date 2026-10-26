#include "rendering/core/SceneRender.h"

SceneRender::SceneRender( Camera& camera, ShadowResources& shadowResources,
    SceneRenderResources& resources, SceneRenderConfig& config,
    SceneRenderState& state, LightSettings& lightSettings,
    DirectionalShadowPass& directionalShadowPass, BrightPrefilterPass& brightPrefilterPass,
    BloomPass& bloomPass,
    PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass,
    GeometryPass& geometryPass, LightingPass& lightingPass,
    GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass,
    SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
    Screenquad& screenQuad, Shader& editShader, OutlinePass& outlinePass, SceneObjectManager& objects)
    : config(config)
    , resources(resources)
    , state(state)
    , brightPrefilterPass(brightPrefilterPass)
    , bloomPass(bloomPass)
    , directionalShadowPass(directionalShadowPass)
    , pointShadowPass(pointShadowPass)
    , spotShadowPass(spotShadowPass)
    , outlinePass(outlinePass)
    , objects(objects)
    , shadowDebugPass(resources, shadowResources)
    , deferredRenderPass(config, geometryPass, ssaoCommonPass, gBuffer, lightingPass)
    , forwardHDRPass(camera, resources, config, state, sphereDrawer, modelDrawer)
    , forwardOverlayPass(camera, resources, state, config)
    , screenPass(config, bloomPass)
    , editScreenPass(screenQuad, editShader, camera)
    {
        (void)lightSettings;
    }

void SceneRender::render( int bfwidth, int bfheight, Shader& screenShader, Screenquad& screenQuad,
    Framebuffer& framebuffer, const SceneObject* selectedObject, unsigned int outputFramebuffer)
{
        directionalShadowPass.render(objects, config.sceneSelection);
        pointShadowPass.render(state.lightPositions, objects, config.sceneSelection);
        spotShadowPass.render(objects, config.sceneSelection);

        if (config.renderMode == RenderMode::ShadowDebug && resources.shaderLibrary)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
            shadowDebugPass.render(bfwidth, bfheight, screenQuad);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return;
        }

        if (config.renderMode == RenderMode::Lighting)
        {
            deferredRenderPass.render(bfwidth, bfheight, framebuffer, screenQuad);
            forwardOverlayPass.render(bfwidth, bfheight, framebuffer);
        }
        else
        {
            forwardHDRPass.render(bfwidth, bfheight, framebuffer, objects);
        }

        if (config.enableBloom && resources.shaderLibrary)
        {
            brightPrefilterPass.setThreshold(config.bloomThreshold);
            brightPrefilterPass.render(framebuffer.getTextureID(0));

            bloomPass.render(brightPrefilterPass.GetBrightPrefilterTextureID());
        }

        if (config.enableEdit)
        {
            editScreenPass.render(bfwidth, bfheight, framebuffer);

            // This outline path copies current-frame depth from the G-buffer.
            if (config.renderMode == RenderMode::Lighting &&
                selectedObject && selectedObject->scene == config.sceneSelection)
            {
                outlinePass.render(selectedObject, bfwidth, bfheight);
            }
        }

        screenPass.render(bfwidth, bfheight, screenShader, screenQuad, framebuffer, outputFramebuffer);
    }
