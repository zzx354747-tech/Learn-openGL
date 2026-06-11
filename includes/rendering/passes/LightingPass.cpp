#include "LightingPass.h"

void LightingPass::render(Framebuffer& framebuffer, Screenquad& screenQuad)
{
    if (!resources.lightingPassShader)
        return;

    framebuffer.bind();
    glDepthMask(GL_FALSE); // 禁止写入深度,保护深度信息
    
    Shader* shader = getLightingShader();   
    shader->use();

    bindGBufferTextures(*shader);
    // 方向光阴影贴图
    shader->setInt("shadowMap", 10);
    // 点光源阴影贴图
    shader->setInt("depthCubeMap", 11);

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

void LightingPass::bindGBufferTextures(Shader& shader)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(0));
    shader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(1));
    shader.setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getGbufferTextureID(2));
    shader.setInt("gAlbedoSpec", 2);
}
