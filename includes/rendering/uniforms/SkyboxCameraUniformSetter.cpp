#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/uniforms/TemporalJitter.h"

void SkyboxCameraUniformSetter::apply( Shader& shader, Camera& camera, int width, int height )
{
        glm::mat4 view =
            glm::mat4(glm::mat3(camera.GetViewMatrix()));

        glm::mat4 projection =
            glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(width) / height,
                0.1f,
                100.0f
            );
        projection = TemporalJitter::apply(projection, width, height);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
    }
