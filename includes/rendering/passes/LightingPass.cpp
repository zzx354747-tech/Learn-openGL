#include "LightingPass.h"

void LightingPass::render(Framebuffer& framebuffer,
    Screenquad& screenQuad,
    unsigned int aoTexture)
{
    if (!resources.lightingPassShader)
        return;

    framebuffer.bind();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // 禁止写入深度,保护深度信息

    Shader* shader = getLightingShader();
    shader->use();

    bindLightingTextures(*shader, aoTexture);
    
    setupObjectLighting(*shader);
    // 使用更高的纹理单元，避免同一个shader内sampler纹理单元冲突
    ShadowMapBinder::apply(*shader, shadowResources, state.dirLightSpaceMatrix, 10);
    // 点光源阴影贴图绑定到更高的纹理单元，避免与其他贴图单元冲突
    setupPointShadow(*shader, 11);
    // 聚光灯阴影贴图绑定到更高的纹理单元，避免与其他贴图单元冲突
    setupSpotShadow(*shader, 12);

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
    shader.setFloat("ssaoStrength", config.ssaoStrength);
    shader.setFloat("bloomThreshold", config.bloomThreshold);

    LightUniformSetter::apply(shader, lightSettings, config, state, camera);
}

void LightingPass::setupPointShadow(Shader& shader, unsigned int textureUnit)
{
    if (!shadowResources.pointShadowMap)
        return;

        PointShadowUniformSetter::apply(shader,
            *shadowResources.pointShadowMap,
            state.lightPositions,
            textureUnit);
}

void LightingPass::setupSpotShadow(Shader& shader, unsigned int textureUnit)
{
    if (!shadowResources.spotShadowMap)
        return;

    SpotShadowUniformSetter::apply(shader,
        *shadowResources.spotShadowMap,
        state.spotLightSpaceMatrix,
        textureUnit);
}

Shader* LightingPass::getLightingShader()
{
    return resources.lightingPassShader;
}

void LightingPass::bindLightingTextures(Shader& shader, unsigned int aoTexture)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(0));
    shader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(1));
    shader.setInt("gNormalRoughness", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(2));
    shader.setInt("gAlbedoMetallic", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, aoTexture);
    shader.setInt("AO", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, resources.irradianceMap->GetID());
    shader.setInt("irradianceMap", 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, resources.prefilterMap->GetID());
    shader.setInt("prefilterMap", 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, resources.brdfLUT->GetID());
    shader.setInt("brdfLUT", 6);
}
