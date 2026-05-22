#include "LightVisualPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

void lightVisualPass::renderLightVisualPass(
    Camera& camera, 
    SceneRenderResources1& resources, 
    SceneRenderState state,
    SceneRenderConfig& config,
    int bfwidth, 
    int bfheight)
{
    if (!resources.lightCubeShader || 
            !resources.lightMesh||
            !config.enablePointLight)
            return;
        resources.lightCubeShader->use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, state.lightPositions);
        model = glm::scale(model, glm::vec3(0.2f)); // 将灯光立方体缩小
        resources.lightCubeShader->setMat4("model", model);
        CameraUniformSetter::apply(*resources.lightCubeShader, camera, bfwidth, bfheight);

        resources.lightMesh->draw();
}
