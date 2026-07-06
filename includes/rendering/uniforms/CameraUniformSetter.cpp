#include "rendering/uniforms/CameraUniformSetter.h"

void CameraUniformSetter::apply( Shader& shader, Camera& camera, int width, int height )
{
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            100.0f
        );

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("cameraPos", camera.Getposition());
    }
