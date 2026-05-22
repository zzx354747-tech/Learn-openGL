#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"

class CameraUniformSetter
{
public:
    static void apply(
        Shader& shader,
        Camera& camera,
        int width,
        int height
    )
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
    }
};
