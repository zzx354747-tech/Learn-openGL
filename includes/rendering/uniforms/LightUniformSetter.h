#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/core/SceneRenderTypes.h"

class LightUniformSetter
{
public:
    static void apply(
        Shader& shader,
        const LightSettings& lightSettings,
        const SceneRenderConfig& config,
        const SceneRenderState& state,
        const Camera& camera
    )
    {
        if (config.enablePointLight)
        {
            setupPointLight(shader, state, lightSettings);
        }

        if (config.enableDirectionalLight)
        {
            setupDirectionalLight(shader, lightSettings);
        }

        if (config.enableFlashlight)
        {
            setupFlashLight(shader, camera, lightSettings);
        }
    }

private:

    static void setupPointLight(
        Shader& shader,
        const SceneRenderState& state,
        const LightSettings& lightSettings
    )
    {
        shader.setVec3("pointLight.position", state.lightPositions);

        shader.setVec3("pointLight.ambient", lightSettings.pointAmbient);
        shader.setVec3("pointLight.diffuse", lightSettings.pointDiffuse);
        shader.setVec3("pointLight.specular", lightSettings.pointSpecular);

        shader.setFloat("pointLight.constant", lightSettings.pointConstant);
        shader.setFloat("pointLight.linear", lightSettings.pointLinear);
        shader.setFloat("pointLight.quadratic", lightSettings.pointQuadratic);
    }

    static void setupDirectionalLight(Shader& shader, const LightSettings& lightSettings)
    {
        shader.setVec3("sun.direction",
            lightSettings.sunDirection);

        shader.setVec3("sun.ambient",
            lightSettings.sunAmbient);

        shader.setVec3("sun.diffuse",
            lightSettings.sunDiffuse);

        shader.setVec3("sun.specular",
            lightSettings.sunSpecular);
    }

    static void setupFlashLight(
        Shader& shader,
        const Camera& camera,
        const LightSettings& lightSettings
    )
    {
        shader.setVec3(
            "flashLight.position",
            getFlashLightPosition(camera, lightSettings)
        );

        shader.setVec3(
            "flashLight.direction",
            camera.GetFront()
        );

        shader.setVec3(
            "flashLight.ambient",
            lightSettings.flashAmbient
        );

        shader.setVec3(
            "flashLight.diffuse",
            lightSettings.flashDiffuse
        );

        shader.setVec3(
            "flashLight.specular",
            lightSettings.flashSpecular
        );

        shader.setFloat(
            "flashLight.constant",
            lightSettings.flashConstant
        );

        shader.setFloat(
            "flashLight.linear",
            lightSettings.flashLinear
        );

        shader.setFloat(
            "flashLight.quadratic",
            lightSettings.flashQuadratic
        );

        shader.setFloat(
            "flashLight.cutOff",
            glm::cos(glm::radians(lightSettings.flashCutOff))
        );

        shader.setFloat(
            "flashLight.outerCutOff",
            glm::cos(glm::radians(lightSettings.flashOuterCutOff))
        );
    }

    static glm::vec3 getFlashLightPosition(
        const Camera& camera,
        const LightSettings& lightSettings
    )
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
