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
            ShadowMapBinder::apply(*shader, shadowResources, state.dirLightSpaceMatrix, 10);
            setupPointShadow(*shader, 11);
            setupSpotShadow(*shader, 12);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        
        if (config.renderMode == RenderMode::Reflection && !resources.skybox)
            return;

        bindCubeDiffuseTexture(*shader, *resources.cubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.cubeNormalTexture);
        bindCubeParallaxTexture(*shader, *resources.cubeParallaxTexture);   

        drawer.drawCubes(*shader);

        bindCubeDiffuseTexture(*shader, *resources.secondCubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.secondCubeNormalTexture);
        bindCubeParallaxTexture(*shader, *resources.secondCubeParallaxTexture);

        drawer.drawSecondCubes(*shader);
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

void SceneObjectPass::renderModel(int bfwidth, int bfheight)
{
    Shader* shader = getModelShader();

    if (!shader)
        return;

    if (config.renderMode == RenderMode::Reflection && !resources.skybox)
        return;

    shader->use();

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

    drawer.drawModel(*shader);
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
    shader.setBool("enableNormalMapping", config.cubeEnableNormalMapping);
    cubeTexture.bind(1);
}

void SceneObjectPass::bindCubeParallaxTexture(Shader& shader, GLTexture& cubeTexture)
{
    glActiveTexture(GL_TEXTURE2);
    shader.setInt("depthMap", 2);
    shader.setFloat("heightScale", config.cubeParallaxHeightScale);
    shader.setInt("numLayers", config.cubeNumLayers);
    shader.setBool("enableParallaxMapping", config.cubeEnableParallaxMapping);
    cubeTexture.bind(2);
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
    shader.setFloat("bloomThreshold", config.bloomThreshold);

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
