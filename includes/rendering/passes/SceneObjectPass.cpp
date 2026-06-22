#include "rendering/passes/SceneObjectPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/uniforms/ShadowMapBinder.h"
#include "rendering/uniforms/PointShadowUniformSetters.h"
#include "rendering/uniforms/SpotShadowUniformSetter.h"

void SceneObjectPass::renderNoLightingCube(int bfwidth, int bfheight)
{
    Shader* shader = getCubeShader();
        
        if (!shader || !resources.cubeMesh || !resources.cubeDiffuseTexture)
            return;
        shader->use();

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        
        if (config.renderMode == RenderMode::Reflection && !resources.skybox)
            return;

        bindCubeTexture(*shader, *resources.cubeDiffuseTexture);
       
        drawer.drawCubes(*shader);

        bindCubeTexture(*shader, *resources.secondCubeDiffuseTexture);

        drawer.drawSecondCubes(*shader);
}

void SceneObjectPass::renderNoLightingPlane(int bfwidth, int bfheight)
{
    Shader* shader = getPlaneShader();

    if (!shader ||
        !resources.planeMesh ||
        !resources.floorTexture ||
        !config.enableFloor)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

    bindPlaneTexture(*shader, *resources.floorTexture);

    drawer.drawPlane(*shader);
}

void SceneObjectPass::renderNoLightingMaterialSpheres(int bfwidth, int bfheight)
{
    Shader* shader = getModelShader();

    if (!shader || !resources.sphereMesh)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        PBRMaterialTextures& material = resources.materialSpherePBRMaterials[i];
        if (!material.albedo)
            continue;

        bindAlbedoTexture(*shader, *material.albedo);
        drawer.drawMaterialSphere(*shader, i);
    }
}

void SceneObjectPass::renderNoLightingModel(int bfwidth, int bfheight)
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

    drawer.drawModel(*shader);
}

void SceneObjectPass::bindCubeTexture(Shader& shader, GLTexture& cubeTexture)
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

void SceneObjectPass::bindPlaneTexture(Shader& shader, GLTexture& floorTexture)
{
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("texture1", 0);
    floorTexture.bind();
}

void SceneObjectPass::bindAlbedoTexture(Shader& shader, GLTexture& albedoTexture)
{
    albedoTexture.bind(0);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("texture_diffuse1", 0);
}

Shader* SceneObjectPass::getPlaneShader()
{
    switch (config.renderMode)
        {
            case RenderMode::Basic:
            return  resources.basicPlaneShader;

            case RenderMode::Lighting:
            return nullptr;

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
            return nullptr;
        
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
            return nullptr;

        case RenderMode::Reflection:
            return resources.basicModelShader;

        default:
            return nullptr;
    }
}
