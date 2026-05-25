#pragma once
#include <glm/glm.hpp>

enum class RenderMode
{
    Basic,
    Lighting,
    Reflection,
    ShadowDebug,
};

struct SceneRenderConfig
{
    bool enableFloor = false;
    bool enableSkybox = false;
    bool enableFlashlight = false;
    bool enablePointLight = false;
    bool enableDirectionalLight = false;

    RenderMode renderMode = RenderMode::Basic;
};

struct SceneRenderState
{
    glm::vec3 cubePositions[3] =
    {
        glm::vec3(-1.0f, 0.0f, -1.0f),
        glm::vec3( 1.5f, 0.0f, -2.5f),
        glm::vec3( 3.8f, 0.0f, -0.8f)
    };

    glm::vec3 lightPositions = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f);

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
