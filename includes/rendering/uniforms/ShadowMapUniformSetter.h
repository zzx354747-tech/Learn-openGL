#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/gl.h>
#include "core/Shader.h"
#include "scene/Camera.h"

class ShadowMapUniformSetter
{
    public:
    static glm::mat4 apply(Shader& shader)
    {
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), 
            glm::vec3(0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        return lightSpaceMatrix;
    }
};
