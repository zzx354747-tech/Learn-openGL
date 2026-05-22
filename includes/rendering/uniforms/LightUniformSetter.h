#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/core/SceneRenderTypes.h"

struct LightSettings
{
    glm::vec3 pointAmbient  = glm::vec3(0.05f);
    glm::vec3 pointDiffuse  = glm::vec3(0.8f);
    glm::vec3 pointSpecular = glm::vec3(1.0f);

    float pointConstant  = 1.0f;
    float pointLinear    = 0.09f;
    float pointQuadratic = 0.032f;

    glm::vec3 sunDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 sunAmbient   = glm::vec3(0.05f);
    glm::vec3 sunDiffuse   = glm::vec3(0.4f);
    glm::vec3 sunSpecular  = glm::vec3(0.5f);

    glm::vec3 flashAmbient  = glm::vec3(0.05f);
    glm::vec3 flashDiffuse  = glm::vec3(0.8f);
    glm::vec3 flashSpecular = glm::vec3(1.0f);

    float flashConstant  = 1.0f;
    float flashLinear    = 0.09f;
    float flashQuadratic = 0.032f;

    float flashCutOff      = 12.5f;
    float flashOuterCutOff = 17.5f;
};

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
            camera.Getposition()
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
};
