#pragma once
#include <glad/gl.h>
#include "core/Shader.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/uniforms/ShadowMapUniformSetter.h"

class ShadowMapBinder
{
public:
    static void apply(Shader& shader, 
        ShadowResources& shadowResources,
        const glm::mat4& lightSpaceMatrix,
        int textureUnit = 1)
    {
        ShadowMapUniformSetter::apply(shader, lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, shadowResources.shadowMap->getDepthMapTexture());
        shader.setInt("shadowMap", textureUnit);
    }
};
