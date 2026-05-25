#pragma once
#include <glm/glm.hpp>
#include "core/Shader.h"

class ShadowMapUniformSetter
{
    public:
    static void apply(Shader& shader, const glm::mat4& lightSpaceMatrix)
    {
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    }
};
