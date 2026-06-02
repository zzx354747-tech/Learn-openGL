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
    bool enableGammaCorrection = false;
    bool enableHDR = false;
    bool enableNormalMapping = false;
    bool enableParallaxMapping = false;
    float parallaxHeightScale = 0.03f;
    int numLayers = 32;
    float exposure = 1.0f;

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

    glm::vec3 secondCubePositions[3] =
    {
        glm::vec3(-1.0f,  1.0f, -6.0f),
        glm::vec3( 1.5f,  1.0f, -8.0f),
        glm::vec3( 3.8f,  1.0f, -5.5f),
    };

    glm::vec3 lightPositions = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f);

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
