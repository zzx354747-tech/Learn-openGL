#include "rendering/uniforms/SSAOCameraUniformSetter.h"
#include "rendering/uniforms/TemporalJitter.h"

void SSAOCameraUniformSetter::apply( Shader& shader, Camera& camera, float width, float height )
{
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            20000.0f
        );
        projection = TemporalJitter::apply(
            projection,
            static_cast<int>(width),
            static_cast<int>(height));

        glm::mat3 normalMatrix = glm::mat3(transpose(inverse(view)));

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat3("normalMatrix", normalMatrix);
        shader.setFloat("screenWidth", width);
        shader.setFloat("screenHeight", height);
    }
