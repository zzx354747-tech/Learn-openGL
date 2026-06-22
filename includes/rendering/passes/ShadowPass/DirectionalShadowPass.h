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
        glm::vec3 sceneCenter = drawer.getActiveSceneWorldCenter();
        glm::vec3 sceneSize = drawer.getActiveSceneWorldSize();
        float sceneExtent = glm::max(sceneSize.x, glm::max(sceneSize.y, sceneSize.z));
        float halfExtent = glm::max(sceneExtent * 0.75f, 10.0f);
        float nearPlane = 0.1f;
        float farPlane = glm::max(sceneExtent * 2.5f, 30.0f);

        glm::mat4 lightProjection = glm::ortho(
            -halfExtent,
            halfExtent,
            -halfExtent,
            halfExtent,
            nearPlane,
            farPlane);

        glm::vec3 lightDirection = glm::normalize(lightSettings.sunDirection);
        glm::mat4 lightView = glm::lookAt(
            sceneCenter - lightDirection * halfExtent,
            sceneCenter,
            glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }
};
