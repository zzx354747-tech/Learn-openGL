#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/postprocess/DirectionalShadowMap.h"

class DirectionalShadowPass
{
public:
    DirectionalShadowPass(
        DirectionalShadowMap& shadowMap,
        Shader& shadowShader,
        SceneDrawer& drawer,
        SceneRenderState& state,
        LightSettings& lightSettings)
        : shadowMap(shadowMap),
        shadowShader(shadowShader),
        drawer(drawer),
        state(state),
        lightSettings(lightSettings)
    {
    }

    void render()
    {
        state.dirLightSpaceMatrix = createLightSpaceMatrix();

        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glViewport(0, 0, shadowMap.getWidth(), shadowMap.getHeight());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);

        drawer.drawScene(shadowShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    DirectionalShadowMap& shadowMap;
    Shader& shadowShader;
    SceneDrawer& drawer;
    SceneRenderState& state;
    LightSettings& lightSettings;

    glm::mat4 createLightSpaceMatrix() const
    {
        float nearPlane = 1.0f;
        float farPlane = 7.5f;

        glm::mat4 lightProjection = glm::ortho(
            -10.0f,
            10.0f,
            -10.0f,
            10.0f,
            nearPlane,
            farPlane);

        glm::vec3 lightDirection = glm::normalize(lightSettings.sunDirection);
        glm::mat4 lightView = glm::lookAt(
            -lightDirection * 4.5f,
            glm::vec3(0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }
};
