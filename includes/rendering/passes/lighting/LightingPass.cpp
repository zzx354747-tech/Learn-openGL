#include "LightingPass.h"
#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/assets/mesh/WaterMesh.h"

#include <chrono>

void LightingPass::render(Framebuffer& framebuffer,
    Screenquad& screenQuad)
{
    if (!resources.shaderLibrary)
        return;

    framebuffer.bind();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // 禁止写入深度,保护深度信息

    Shader* shader = getLightingShader();
    shader->use();
    bindLightingInputTextures(*shader);

    setupObjectLighting(*shader);
    setupPointShadowUniform(*shader);
    setupSpotShadowUniform(*shader);

    // 绘制屏幕四边形
    screenQuad.draw();

    // 在draw时如果进行深度测试，会覆盖掉之前的深度信息
    // 在draw之后恢复深度写入，确保后续的skybox和light render能够正确使用深度测试
    glDepthMask(GL_TRUE);
    framebuffer.unbind();
}

void LightingPass::setupObjectLighting(Shader& shader)
{
    shader.setVec3("viewPos", camera.Getposition());
    shader.setBool("enablePointLight", config.enablePointLight);
    shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
    shader.setBool("enableFlashlight", config.enableFlashlight);
    shader.setBool("enableSSAO", config.enableSSAO);
    shader.setBool("enablePBR", config.enablePBR);
    shader.setBool("enableIBL", config.enableIBL);
    shader.setBool("enableGI", config.enableGI);
    shader.setBool("enableWaterCaustics",
                   config.enableWater &&
                   config.sceneSelection == SceneSelection::FujiTerrain);
    shader.setFloat("ssaoStrength", config.ssaoStrength);
    shader.setFloat("giStrength", config.giStrength);
    shader.setFloat("giRadius", config.giRadius);
    shader.setFloat("giMaxDistance", config.giMaxDistance);
    shader.setInt("giSampleCount", config.giSampleCount);
    shader.setVec3("fixedAmbientColor", config.fixedAmbientColor);
    shader.setFloat("fixedAmbientStrength", config.fixedAmbientStrength);
    shader.setVec3("iblAmbientTint", config.iblAmbientTint);
    shader.setFloat("iblAmbientStrength", config.iblAmbientStrength);
    shader.setFloat("cloudAmbientTransmission",
                    calculateCloudAmbientTransmission(config));
    shader.setBool("enableStormShaftLighting",
                   shouldRenderGodRays(config) &&
                   shouldRenderVolumetricClouds(config) &&
                   config.stormHoleStrength > 0.001f);
    shader.setVec2("stormHeroHolePosition", config.stormHeroHolePosition);
    shader.setVec2("stormShaftLean", config.stormShaftLean);
    shader.setFloat("stormHoleSize", config.stormHoleSize);
    shader.setFloat("stormPoolHoleSize", config.stormPoolHoleSize);
    shader.setFloat("stormHoleSoftness", config.stormHoleSoftness);
    shader.setVec3("stormShaftColor", config.godRayColor);
    shader.setFloat("stormShaftSurfaceIntensity",
                    config.stormHoleStrength * config.stormHoleShaftStrength *
                    config.godRayIntensity);
    shader.setFloat("phongDiffuseStrength", config.phongDiffuseStrength);
    shader.setFloat("phongSpecularStrength", config.phongSpecularStrength);
    shader.setFloat("phongIBLDiffuseStrength", config.phongIBLDiffuseStrength);
    shader.setFloat("phongIBLSpecularStrength", config.phongIBLSpecularStrength);
    shader.setFloat("bloomThreshold", config.bloomThreshold);
    shader.setFloat("pointShadowStrength", lightSettings.pointShadowStrength);
    shader.setFloat("sunShadowStrength", lightSettings.sunShadowStrength);
    shader.setFloat("flashShadowStrength", lightSettings.flashShadowStrength);
    shader.setFloat("directionalShadowLightSize", config.directionalShadowLightSize);
    shader.setFloat("directionalShadowBlockerSearchRadius", config.directionalShadowBlockerSearchRadius);
    shader.setFloat("directionalShadowMinFilterRadius", config.directionalShadowMinFilterRadius);
    shader.setFloat("directionalShadowMaxFilterRadius", config.directionalShadowMaxFilterRadius);
    shader.setFloat("directionalShadowBiasSlope", config.directionalShadowBiasSlope);
    shader.setFloat("directionalShadowBiasMin", config.directionalShadowBiasMin);
    shader.setFloat("waterLevel", TerrainMesh::WaterLevel);
    shader.setVec2("waterCenter", glm::vec2(0.0f, WaterMesh::CenterZ));
    shader.setVec2("waterRadii", glm::vec2(WaterMesh::RadiusX, WaterMesh::RadiusZ));
    static const auto waterAnimationStart = std::chrono::steady_clock::now();
    shader.setFloat("waterTime", std::chrono::duration<float>(
        std::chrono::steady_clock::now() - waterAnimationStart).count());
    shader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);

    LightUniformSetter::apply(shader, lightSettings, config, state, camera);
}

void LightingPass::setupPointShadowUniform(Shader& shader)
{
    shader.setVec3("lightPos", state.lightPositions);

    if (shadowResources.pointShadowMap)
    {
        shader.setFloat("farPlane", shadowResources.pointShadowMap->getFarPlane());
    }
}

void LightingPass::setupSpotShadowUniform(Shader& shader)
{
    shader.setMat4("spotLightSpaceMatrix", state.spotLightSpaceMatrix);
}

Shader* LightingPass::getLightingShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->lightingPass : nullptr;
}

void LightingPass::bindLightingInputTextures(Shader& shader)
{
    const auto& handles = resources.lightingHandles;
    auto& registry = resources.registry;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gPosition));
    shader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gNormalRoughness));
    shader.setInt("gNormalRoughness", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gAlbedoMetallic));
    shader.setInt("gAlbedoMetallic", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.ao));
    shader.setInt("AO", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.shadowMap));
    shader.setInt("shadowMap", 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.depthCubeMap));
    shader.setInt("depthCubeMap", 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.spotShadowMap));
    shader.setInt("spotShadowMap", 6);

    // Keep unit 7 empty. Some Apple OpenGL drivers retain a stale cube target on
    // this unit and report it as unloadable when a float sampler is assigned.
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.brdfLUT));
    shader.setInt("brdfLUT", 10);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.irradianceMap));
    shader.setInt("irradianceMap", 8);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.prefilterMap));
    shader.setInt("prefilterMap", 9);
}

LightingPass::LightingPass( SceneRenderResources& resources, ShadowResources& shadowResources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, Camera& camera ) : resources(resources), shadowResources(shadowResources), config(config), state(state), lightSettings(lightSettings), camera(camera)
{}
