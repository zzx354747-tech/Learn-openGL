#include "LightVisualPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

namespace
{
    void drawLightCube(
        Shader& shader,
        LightMesh& lightMesh,
        const glm::vec3& position,
        const glm::vec3& color,
        float scale)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(scale));
        shader.setMat4("model", model);
        shader.setVec3("lightColor", color);
        lightMesh.draw();
    }
}

void lightVisualPass::renderLightVisualPass(
    Camera& camera, 
    SceneRenderResources& resources,
    SceneRenderState state,
    SceneRenderConfig& config,
    int bfwidth, 
    int bfheight)
    {
        if (!resources.lightCubeShader || 
            !resources.lightMesh)
        {
            std::cerr << "LightVisualPass: Missing resources for light visualization." << std::endl;
            return;
        }

        resources.lightCubeShader->use();
        CameraUniformSetter::apply(*resources.lightCubeShader, camera, bfwidth, bfheight);

        if (config.enablePointLight)
        {
            drawLightCube(
                *resources.lightCubeShader,
                *resources.lightMesh,
                state.lightPositions,
                glm::vec3(6.0f),
                0.2f);
        }
    }
