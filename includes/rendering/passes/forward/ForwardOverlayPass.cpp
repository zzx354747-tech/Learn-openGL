#include "rendering/passes/forward/ForwardOverlayPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

#include <chrono>

void ForwardOverlayPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer
)
{
    framebuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glEnable(GL_DEPTH_TEST);

    renderWater(bfwidth, bfheight);

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
        lightSettings,
        bfwidth,
        bfheight
    );

    framebuffer.unbind();
}

ForwardOverlayPass::ForwardOverlayPass(Camera& camera, SceneRenderResources& resources,
    SceneRenderState& state, SceneRenderConfig& config, LightSettings& lightSettings)
    : camera(camera), resources(resources), state(state), config(config), lightSettings(lightSettings)
{
}

void ForwardOverlayPass::renderWater(int bfwidth, int bfheight)
{
    if (!config.enableWater || config.sceneSelection != SceneSelection::FujiTerrain ||
        !resources.waterMesh || !resources.shaderLibrary)
        return;

    Shader& shader = resources.shaderLibrary->water;
    shader.use();
    CameraUniformSetter::apply(shader, camera, bfwidth, bfheight);

    static const auto startTime = std::chrono::steady_clock::now();
    const float time = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime).count();
    shader.setFloat("waterTime", time);
    shader.setVec2("viewportSize", glm::vec2(static_cast<float>(bfwidth), static_cast<float>(bfheight)));
    shader.setVec3("sunDirection", lightSettings.sunDirection);
    shader.setVec3("sunColor", lightSettings.sunDiffuse * lightSettings.sunIntensity *
                                  lightSettings.sunIntensityScale *
                                  calculateCloudSunTransmission(config));
    shader.setFloat("cloudAmbientTransmission",
                    calculateCloudAmbientTransmission(config));
    shader.setBool("enableStormShaftLighting",
                   shouldRenderGodRays(config) &&
                   shouldRenderVolumetricClouds(config) &&
                   config.stormHoleStrength > 0.001f);
    shader.setVec2("stormShaftLean", config.stormShaftLean);
    shader.setInt("stormHoleSeed", static_cast<int>(config.stormHoleSeed));
    shader.setInt("stormHoleCount", config.stormHoleCount);
    shader.setFloat("stormHoleMinRadius", config.stormHoleMinRadius);
    shader.setFloat("stormHoleMaxRadius", config.stormHoleMaxRadius);
    shader.setFloat("stormHoleSoftness", config.stormHoleSoftness);
    shader.setVec3("stormShaftColor", config.godRayColor);
    shader.setFloat("stormShaftSurfaceIntensity",
                    config.stormHoleStrength * config.stormHoleShaftStrength *
                    config.godRayIntensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
        resources.prefilterMap ? resources.prefilterMap->GetID() : 0);
    shader.setInt("prefilterMap", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D,
        resources.registry.resolveTexture(resources.lightingHandles.gPosition));
    shader.setInt("gPosition", 1);

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    resources.waterMesh->draw(shader);
    glDepthMask(GL_TRUE);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);
}
