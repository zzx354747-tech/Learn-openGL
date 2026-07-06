#include "PointShadowPass.h"

void PointShadowPass::render(const glm::vec3& lightpos)
{
    float nearPlane = pointShadowMap.getNearPlane();
    float farPlane = pointShadowMap.getFarPlane();

    std::vector<glm::mat4> shadowTransforms = createShadowTransforms(
        lightpos,
        nearPlane, 
        farPlane);

    shadowShader.use();
    // 把六个视图矩阵传给shader
    for (unsigned int i = 0; i < 6; ++i)
    {
        shadowShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", 
        shadowTransforms[i]);
    }
    shadowShader.setFloat("farPlane", farPlane);
    shadowShader.setVec3("lightPos", lightpos);

    // 正常的fbo渲染流程
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glViewport(0, 0,pointShadowMap.getWidth(),pointShadowMap.getHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowMap.getFBO());
    glClear(GL_DEPTH_BUFFER_BIT);

    sphereDrawer.draw(shadowShader);
    modelDrawer.draw(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 将渲染好的深度立方体贴图绑定到资源注册表中
    registry.setTexture(depthCubeMapHandle, pointShadowMap.getDepthCubeMap());
}

std::vector<glm::mat4> PointShadowPass::createShadowTransforms(const glm::vec3& lightPos, 
    float nearPlane, 
    float farPlane)
{
    float aspect = 1.0f; // 立方体贴图的宽高比为1
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 
    aspect, 
    nearPlane, 
    farPlane);

    std::vector<glm::mat4> matrices;
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(1.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, -1.0f, 0.0f)));
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, -1.0f, 0.0f)));
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(0.0f, 1.0f, 0.0f), 
        glm::vec3(0.0f, 0.0f, 1.0f)));
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(0.0f, -1.0f, 0.0f), 
        glm::vec3(0.0f, 0.0f, -1.0f)));
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(0.0f, 0.0f, 1.0f), 
        glm::vec3(0.0f, -1.0f, 0.0f)));
    matrices.push_back(shadowProj * glm::lookAt(lightPos, 
        lightPos + glm::vec3(0.0f, 0.0f, -1.0f), 
        glm::vec3(0.0f, -1.0f, 0.0f)));
    return matrices;
}

PointShadowPass::PointShadowPass(PointShadowMap& shadowMap, Shader& shadowShader, 
    SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
    ResourceRegistry& registry, ResourceHandle depthCubeMapHandle)
    :pointShadowMap(shadowMap), shadowShader(shadowShader), 
    sphereDrawer(sphereDrawer), modelDrawer(modelDrawer),
    registry(registry), depthCubeMapHandle(depthCubeMapHandle)
{}
