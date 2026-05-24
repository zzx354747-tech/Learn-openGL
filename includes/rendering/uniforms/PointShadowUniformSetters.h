#pragma once

#include <glad/gl.h>
#include "core/Shader.h"
#include "rendering/postprocess/PointShadowMap.h"

class PointShadowUniformSetter
{
public:
    static void apply(
        Shader& shader,
        PointShadowMap& shadowMap,
        const glm::vec3& lightPos,
        unsigned int textureUnit = 2
    )
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.getDepthCubeMap());

        shader.setInt("depthCubeMap", textureUnit);
        shader.setVec3("lightPos", lightPos);
        shader.setFloat("farPlane", shadowMap.getFarPlane());
    }
};
