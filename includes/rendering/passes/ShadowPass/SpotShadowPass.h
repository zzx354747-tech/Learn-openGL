#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/postprocess/SpotShadowMap.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/core/SceneRenderTypes.h"

class SpotShadowPass
{
public:
    SpotShadowPass(
        SpotShadowMap& spotShadowMap,
        Shader& spotShadowShader,
        SceneDrawer& drawer,
        Camera& camera,
        SceneRenderState& state,
        LightSettings& lightSettings)
        : spotShadowMap(spotShadowMap),
        spotShadowShader(spotShadowShader),
        drawer(drawer),
        camera(camera),
        state(state),
        lightSettings(lightSettings)
    {
    }

    void render()
    {
        lightSpaceMatrix = createLightSpaceMatrix();
        state.spotLightSpaceMatrix = lightSpaceMatrix;

        spotShadowShader.use();
        spotShadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glViewport(
            0,
            0,
            spotShadowMap.getWidth(),
            spotShadowMap.getHeight()
        );

        glBindFramebuffer(GL_FRAMEBUFFER, spotShadowMap.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);

        drawer.drawScene(spotShadowShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    SpotShadowMap& spotShadowMap;
    Shader& spotShadowShader;
    SceneDrawer& drawer;
    Camera& camera;
    SceneRenderState& state;
    LightSettings& lightSettings;

    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

    glm::mat4 createLightSpaceMatrix()
    {
        glm::mat4 lightProjection = glm::perspective(
            glm::radians(45.0f),
            1.0f,
            spotShadowMap.getNearPlane(),
            spotShadowMap.getFarPlane()
        );

        glm::vec3 lightPos = getFlashLightPosition();
        glm::vec3 lightDir = camera.GetFront();

        glm::mat4 lightView = glm::lookAt(
            lightPos,
            lightPos + lightDir,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        return lightProjection * lightView;
    }

    glm::vec3 getFlashLightPosition() const
    {
        glm::vec3 front = camera.GetFront();
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        return camera.Getposition()
            + right * lightSettings.flashRightOffset
            + up * lightSettings.flashUpOffset
            + front * lightSettings.flashForwardOffset;
    }
};
