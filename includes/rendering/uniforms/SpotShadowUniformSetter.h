#pragma once

#include <glad/gl.h>
#include "core/Shader.h"
#include "rendering/postprocess/SpotShadowMap.h"

class SpotShadowUniformSetter
{
public:
    static void apply(
        Shader& shader,
        SpotShadowMap& spotShadowMap,
        const glm::mat4& spotLightSpaceMatrix,
        unsigned int textureUnit = 3
    )
    {
        shader.setMat4(
            "spotLightSpaceMatrix",
            spotLightSpaceMatrix
        );

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(
            GL_TEXTURE_2D,
            spotShadowMap.getDepthMap()
        );

        shader.setInt("spotShadowMap", textureUnit);
    }
};
