#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/TemporalJitter.h"

void CameraUniformSetter::apply( Shader& shader, Camera& camera, int width, int height )
{
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection = glm::perspective(
            glm::radians(Camera::DefaultFieldOfViewDegrees),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            20000.0f
        );
        projection = TemporalJitter::apply(projection, width, height);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("cameraPos", camera.Getposition());
    }
