#include "rendering/passes/SceneObjectPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/uniforms/ShadowMapBinder.h"
#include "rendering/uniforms/PointShadowUniformSetters.h"
#include "rendering/uniforms/SpotShadowUniformSetter.h"

void SceneObjectPass::renderCube(int bfwidth, int bfheight)
{
    Shader* shader = getCubeShader();
        
        if (!shader || !resources.cubeMesh || !resources.cubeDiffuseTexture)
            return;
        shader->use();

        if (config.renderMode == RenderMode::Lighting)
        {
            setupObjectLighting(*shader);
            ShadowMapBinder::apply(*shader, shadowResources, state.dirLightSpaceMatrix);
            setupPointShadow(*shader);
            setupSpotShadow(*shader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        
        if (config.renderMode == RenderMode::Reflection && !resources.skybox)
            return;

        bindCubeDiffuseTexture(*shader, *resources.cubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.cubeNormalTexture);

        drawer.drawCubes(*shader);
}

void SceneObjectPass::renderPlane(int bfwidth, int bfheight)
{
    Shader* shader = getPlaneShader();
    
        if (!shader ||
            !resources.planeMesh||
            !resources.floorTexture||
            !config.enableFloor)
            return;

        shader->use();

        if (config.renderMode == RenderMode::Lighting)
        {
            setupObjectLighting(*shader);
            ShadowMapBinder::apply(*shader, shadowResources, state.dirLightSpaceMatrix);
            setupPointShadow(*shader);
            setupSpotShadow(*shader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        bindPlaneTexture(*shader, *resources.floorTexture);

        drawer.drawPlane(*shader);
}

void SceneObjectPass::renderModel(Model& model, int bfwidth, int bfheight)
{
    Shader* shader = getModelShader();

    if (!shader)
        return;

    if (config.renderMode == RenderMode::Reflection && !resources.skybox)
        return;

    shader->use();
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    if (model.hasValidBounds())
    {
        glm::vec3 boundsCenter = model.getBoundsCenter();
        glm::vec3 boundsSize = model.getBoundsSize();
        float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
        float scale = maxExtent > 0.0f ? 10.0f / maxExtent : 1.0f;
        float floorY = -0.5f;
        glm::vec3 targetCenter(
            0.0f,
            floorY + boundsSize.y * scale * 0.5f,
            -2.0f
        );

        modelMatrix = glm::translate(modelMatrix, targetCenter);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
        modelMatrix = glm::translate(modelMatrix, -boundsCenter);
    }
    shader->setMat4("model", modelMatrix);

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

    if (config.renderMode == RenderMode::Reflection)
    {
        shader->setBool("isSkybox", false);
        shader->setInt("skybox", 0);
        shader->setVec3("cameraPos", camera.Getposition());
        resources.skybox->bind();
    }

    if (config.renderMode == RenderMode::Lighting)
    {
        shader->setInt("shadowMap", 10);
        shader->setInt("depthCubeMap", 11);
        setupObjectLighting(*shader);
        // 使用更高的纹理单元，避免同一个shader内sampler纹理单元冲突
        ShadowMapBinder::apply(*shader, shadowResources, state.dirLightSpaceMatrix, 10);
        // 点光源阴影贴图绑定到更高的纹理单元，避免与其他贴图单元冲突
        setupPointShadow(*shader, 11);
        // 聚光灯阴影贴图绑定到更高的纹理单元，避免与其他贴图单元冲突
        setupSpotShadow(*shader, 12);
    }

    model.draw(*shader);
}

void SceneObjectPass::bindCubeDiffuseTexture(Shader& shader, GLTexture& cubeTexture)
{
   if (config.renderMode == RenderMode::Reflection)
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setBool("isSkybox", false);
            shader.setInt("skybox", 0);
            shader.setVec3("cameraPos", camera.Getposition());
            resources.skybox->bind();
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("texture1", 0);
            cubeTexture.bind();
        }
}

void SceneObjectPass::bindCubeNormalTexture(Shader& shader, GLTexture& cubeTexture)
{
    glActiveTexture(GL_TEXTURE1);
    shader.setInt("normalMap", 1);
    shader.setBool("enableNormalMapping", true);
    cubeTexture.bind(1);
}

void SceneObjectPass::bindPlaneTexture(Shader& shader, GLTexture& floorTexture)
{
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture1", 0);
    floorTexture.bind();
}

void SceneObjectPass::setupObjectLighting(Shader& shader)
{
    shader.setVec3("viewPos", camera.Getposition());
    shader.setBool("enablePointLight", config.enablePointLight);
    shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
    shader.setBool("enableFlashlight", config.enableFlashlight);

    LightUniformSetter::apply(shader, lightSettings, config, state, camera);
}

// 设置第二次渲染时的点光源阴影贴图和相关uniform
void SceneObjectPass::setupPointShadow(Shader& shader, unsigned int textureUnit)
{
    if (!shadowResources.pointShadowMap)
        return;
        
        PointShadowUniformSetter::apply(shader, 
            *shadowResources.pointShadowMap, 
            state.lightPositions,
            textureUnit);
}

void SceneObjectPass::setupSpotShadow(Shader& shader, unsigned int textureUnit)
{
    if (!shadowResources.spotShadowMap)
        return;

    SpotShadowUniformSetter::apply(shader, 
        *shadowResources.spotShadowMap,
        state.spotLightSpaceMatrix,
        textureUnit);
}

Shader* SceneObjectPass::getPlaneShader()
{
    switch (config.renderMode)
        {
            case RenderMode::Basic:
            return  resources.basicPlaneShader;

            case RenderMode::Lighting:
            return resources.lightingPlaneShader;

            case RenderMode::Reflection:
            return resources.basicPlaneShader;

            default:
            return nullptr;
        }
}

Shader* SceneObjectPass::getCubeShader()
{
    switch (config.renderMode)
        {
            case RenderMode::Basic:
            return  resources.basicCubeShader;

            case RenderMode::Lighting:
            return resources.lightingCubeShader;
        
            case RenderMode::Reflection:
            return resources.reflectShader;
            
            default:
            return nullptr;
        }
}

Shader* SceneObjectPass::getModelShader()
{
    switch (config.renderMode)
    {
        case RenderMode::Basic:
            return resources.basicModelShader;

        case RenderMode::Lighting:
            return resources.lightingModelShader;

        case RenderMode::Reflection:
            return resources.reflectModelShader;

        default:
            return nullptr;
    }
}
