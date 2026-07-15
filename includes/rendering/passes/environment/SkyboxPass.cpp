#include "SkyboxPass.h"

void SkyboxPass::renderSkyboxPass(
    Camera& camera, 
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    LightSettings& lightSettings,
    int bfwidth, 
    int bfheight)
{
    if (!resources.skyboxMesh||
            !resources.skybox||
            !config.enableSkybox||
            !resources.shaderLibrary)
            return;

    Shader& shader = resources.shaderLibrary->cubemap;
    shader.use();

    shader.setInt("skybox", 0);
    bindSkyboxTexture(resources.skybox);
    shader.setBool("enableProceduralSky", config.enableProceduralSky);
    shader.setBool(
        "enableVolumetricClouds",
        shouldRenderVolumetricClouds(config));
    shader.setBool("enableSunTexture", config.enableSunTexture && resources.sunTexture != 0);
    shader.setVec3("skyTopColor", config.skyTopColor);
    shader.setVec3("sunDirection", glm::normalize(-lightSettings.sunDirection));
    shader.setVec3("cameraPos", camera.Getposition());
    shader.setFloat("cloudCoverage", config.cloudCoverage);
    shader.setFloat("cloudDensity", config.cloudDensity);
    shader.setFloat("cloudBaseHeight", config.cloudBaseHeight);
    shader.setFloat("cloudThickness", config.cloudThickness);
    shader.setFloat("cloudScale", config.cloudScale);
    shader.setFloat("cloudDetailScale", config.cloudDetailScale);
    shader.setFloat("cloudType", config.cloudType);
    shader.setFloat("cloudAnvilAmount", config.cloudAnvilAmount);
    shader.setFloat("cloudErosionStrength", config.cloudErosionStrength);
    shader.setFloat("stormHoleStrength", config.stormHoleStrength);
    shader.setFloat("stormHoleSize", config.stormHoleSize);
    shader.setFloat("stormPoolHoleSize", config.stormPoolHoleSize);
    shader.setFloat("stormHoleSpacing", config.stormHoleSpacing);
    shader.setFloat("stormHoleSoftness", config.stormHoleSoftness);
    shader.setFloat("stormHoleShaftStrength", config.stormHoleShaftStrength);
    shader.setVec2("stormHeroHolePosition", config.stormHeroHolePosition);
    shader.setVec2("stormShaftLean", config.stormShaftLean);
    shader.setFloat("cloudEvolutionTime", config.cloudEvolutionPhase);
    shader.setVec2("cloudWindOffset", config.cloudAnimationOffset);
    shader.setVec2("cloudWindDirection", config.cloudWindDirection);
    shader.setFloat("cloudWindShear", config.cloudWindShear);
    shader.setFloat("cloudExtinction", config.cloudExtinction);
    shader.setFloat("cloudLightAbsorption", config.cloudLightAbsorption);
    shader.setFloat("cloudAmbientStrength", config.cloudAmbientStrength);
    shader.setFloat("cloudPowderStrength", config.cloudPowderStrength);
    shader.setFloat("cloudMultiScattering", config.cloudMultiScattering);
    shader.setFloat("cloudSilverLining", config.cloudSilverLining);
    shader.setFloat("cloudForwardScattering", config.cloudForwardScattering);
    shader.setFloat("cloudBackwardScattering", config.cloudBackwardScattering);
    shader.setVec3("cloudSunColor", config.cloudSunColor);
    shader.setVec3("cloudBottomColor", config.cloudBottomColor);
    shader.setVec3("cloudTopColor", config.cloudTopColor);
    shader.setInt("cloudViewSteps", config.cloudViewSteps);
    shader.setInt("cloudLightSteps", config.cloudLightSteps);
    shader.setFloat("cloudMaxDistance", config.cloudMaxDistance);
    shader.setFloat("sunAngularRadius", config.sunAngularRadius);
    static unsigned int cloudFrameIndex = 0;
    shader.setInt("cloudFrameIndex", config.enableTAA
        ? static_cast<int>(cloudFrameIndex++ & 15u)
        : 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, resources.sunTexture);
    shader.setInt("sunTexture", 1);

    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    SkyboxCameraUniformSetter::apply(shader, camera, bfwidth, bfheight);

    shader.setBool("isSkybox", true);
    resources.skyboxMesh->draw();

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE); 
    if (cullFaceWasEnabled)
        glEnable(GL_CULL_FACE);

}

void SkyboxPass::bindSkyboxTexture(EnvCubemap* skybox)
{
    glActiveTexture(GL_TEXTURE0);
    skybox->bind();
}
