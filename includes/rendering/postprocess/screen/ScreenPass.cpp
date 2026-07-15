#include "rendering/postprocess/screen/ScreenPass.h"

#include <glm/gtc/matrix_transform.hpp>
#include "rendering/uniforms/TemporalJitter.h"

void ScreenPass::render(
    int bfwidth,
    int bfheight,
    Shader& screenShader,
    Screenquad& screenQuad,
    Framebuffer& framebuffer,
    GLuint sceneTexture
)
{
    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(config.skyTopColor.r, config.skyTopColor.g, config.skyTopColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    screenShader.use();

    screenShader.setInt("screenTexture", 0);
    screenShader.setInt("bloomBlur", 1);
    screenShader.setInt("godRaySource", 2);
    screenShader.setInt("sceneDepth", 3);

    screenShader.setFloat("exposure", config.exposure);
    screenShader.setBool("enableHdr", config.enableHDR);
    screenShader.setBool("enableGamma", config.enableGammaCorrection);
    screenShader.setBool("enableBloom", config.enableBloom);
    screenShader.setFloat("bloomStrength", config.bloomStrength);

    const glm::vec3 worldSunDirection = glm::normalize(-lightSettings.sunDirection);
    const glm::vec3 viewSunDirection = glm::mat3(camera.GetViewMatrix()) * worldSunDirection;
    const glm::mat4 sunProjection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(bfwidth) / static_cast<float>(bfheight),
        0.1f,
        100.0f);
    const glm::vec4 sunClip = sunProjection * glm::vec4(viewSunDirection, 1.0f);
    glm::vec2 sunScreenPos(0.5f);
    if (sunClip.w > 0.0001f)
        sunScreenPos = glm::vec2(sunClip) / sunClip.w * 0.5f + 0.5f;
    const float radius = config.godRayRadius;
    const bool sunCanContribute = viewSunDirection.z < 0.0f &&
        sunScreenPos.x > -radius && sunScreenPos.x < 1.0f + radius &&
        sunScreenPos.y > -radius && sunScreenPos.y < 1.0f + radius;

    const bool godRaysActive = shouldRenderGodRays(config);
    const bool volumetricCloudsActive =
        shouldRenderVolumetricClouds(config);
    const float screenSpaceGodRayMix =
        1.0f - glm::clamp(config.stormHoleStrength, 0.0f, 1.0f);
    screenShader.setBool("enableGodRays", godRaysActive &&
        config.enableSunTexture && sunCanContribute &&
        screenSpaceGodRayMix > 0.001f);
    screenShader.setBool("enableStormGodRays", godRaysActive &&
        volumetricCloudsActive && config.stormHoleStrength > 0.001f);
    screenShader.setVec2("sunScreenPos", sunScreenPos);
    screenShader.setFloat("godRayIntensity", config.godRayIntensity);
    screenShader.setFloat("screenSpaceGodRayMix", screenSpaceGodRayMix);
    screenShader.setFloat("godRayDensity", config.godRayDensity);
    screenShader.setFloat("godRayDecay", config.godRayDecay);
    screenShader.setFloat("godRayWeight", config.godRayWeight);
    screenShader.setFloat("godRayExposure", config.godRayExposure);
    screenShader.setFloat("godRayRadius", config.godRayRadius);
    screenShader.setInt("godRaySamples", config.godRaySamples);
    screenShader.setVec3("godRayColor", config.godRayColor);
    screenShader.setVec2("invResolution", glm::vec2(
        1.0f / static_cast<float>(bfwidth),
        1.0f / static_cast<float>(bfheight)));

    glm::mat4 cameraProjection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(bfwidth) / static_cast<float>(bfheight),
        0.1f,
        400.0f);
    cameraProjection = TemporalJitter::apply(
        cameraProjection, bfwidth, bfheight);
    screenShader.setMat4(
        "inverseViewProjection",
        glm::inverse(cameraProjection * camera.GetViewMatrix()));
    screenShader.setVec3("cameraPos", camera.Getposition());
    screenShader.setFloat("stormHoleStrength", config.stormHoleStrength);
    screenShader.setFloat("stormHoleSize", config.stormHoleSize);
    screenShader.setFloat("stormPoolHoleSize", config.stormPoolHoleSize);
    screenShader.setFloat("stormHoleSpacing", config.stormHoleSpacing);
    screenShader.setFloat("stormHoleSoftness", config.stormHoleSoftness);
    screenShader.setFloat("stormHoleShaftStrength", config.stormHoleShaftStrength);
    screenShader.setVec2("stormHeroHolePosition", config.stormHeroHolePosition);
    screenShader.setVec2("stormShaftLean", config.stormShaftLean);
    screenShader.setFloat("cloudBaseHeight", config.cloudBaseHeight);
    screenShader.setFloat("cloudThickness", config.cloudThickness);
    screenShader.setFloat("cloudScale", config.cloudScale);
    screenShader.setFloat("cloudEvolutionTime", config.cloudEvolutionPhase);
    screenShader.setVec2("cloudWindOffset", config.cloudAnimationOffset);
    screenShader.setVec2("cloudWindDirection", config.cloudWindDirection);
    screenShader.setFloat("cloudWindShear", config.cloudWindShear);
    screenShader.setFloat("cloudMaxDistance", config.cloudMaxDistance);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    glActiveTexture(GL_TEXTURE1);
    if (config.enableBloom && resources.pingpongFBO)
    {
        glBindTexture(
            GL_TEXTURE_2D,
            resources.pingpongFBO->getTextureID(1)
        );
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(0));
    }

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(1));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, framebuffer.getDepthTextureID());

    screenQuad.draw();
}

ScreenPass::ScreenPass(SceneRenderConfig& config, SceneRenderResources& resources,
                       Camera& camera, LightSettings& lightSettings)
    : config(config), resources(resources), camera(camera), lightSettings(lightSettings)
{
    }
