#include "SkyboxPass.h"

void SkyboxPass::renderSkyboxPass(
    Camera& camera, 
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    int bfwidth, 
    int bfheight)
{
    if (!resources.skyboxMesh||
            !resources.skybox||
            !config.enableSkybox||
            !resources.reflectShader)
            return;

    Shader& shader = *resources.reflectShader;
    shader.use();

    shader.setInt("skybox", 0);
    bindSkyboxTexture(resources.skybox);

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
