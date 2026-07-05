#pragma once
#include <glm/glm.hpp>

constexpr unsigned int MaterialSphereCount = 8;

enum class RenderMode
{
    Basic,
    Lighting,
    Reflection,
    ShadowDebug,
};

enum class SceneSelection
{
    Default,
    ModernCity,
};

enum class EnvironmentSelection
{
    Night,
    Sunny,
    NightN8_3K,
};

struct SceneRenderConfig
{
    bool enableSkybox = false;
    bool enableFlashlight = false;
    bool enablePointLight = false;
    bool enableDirectionalLight = false;
    bool enableGammaCorrection = false;
    bool enableHDR = false;
    bool enableBloom = false;
    bool enableSSAO = true;
    bool enablePBR = false;
    bool enableIBL = false;
    float ssaoStrength = 1.0f;
    glm::vec3 fixedAmbientColor = glm::vec3(0.08f);
    float fixedAmbientStrength = 1.0f;
    glm::vec3 iblAmbientTint = glm::vec3(1.0f);
    float iblAmbientStrength = 1.0f;
    float phongDiffuseStrength = 0.55f;
    float phongSpecularStrength = 0.18f;
    float phongIBLDiffuseStrength = 1.25f;
    float phongIBLSpecularStrength = 0.35f;

    int numBlurPasses = 10;
    float exposure = 1.0f;
    float bloomStrength = 0.6f;
    float bloomThreshold = 0.7f;
    float sunThreshold = 100.0f;
    float directionalShadowLightSize = 0.006f;
    float directionalShadowBlockerSearchRadius = 0.006f;
    float directionalShadowMinFilterRadius = 0.0005f;
    float directionalShadowMaxFilterRadius = 0.014f;

    RenderMode renderMode = RenderMode::Basic;
    SceneSelection sceneSelection = SceneSelection::Default;
    EnvironmentSelection environmentSelection = EnvironmentSelection::Night;
};

struct SceneRenderState
{
    glm::vec3 materialSpherePositions[MaterialSphereCount] =
    {
        glm::vec3(-2.4f, 0.0f, -3.9f),
        glm::vec3(-0.8f, 0.0f, -3.9f),
        glm::vec3( 0.8f, 0.0f, -3.9f),
        glm::vec3( 2.4f, 0.0f, -3.9f),
        glm::vec3(-2.4f, 0.0f, -5.7f),
        glm::vec3(-0.8f, 0.0f, -5.7f),
        glm::vec3( 0.8f, 0.0f, -5.7f),
        glm::vec3( 2.4f, 0.0f, -5.7f),
    };

    glm::vec3 lightPositions = glm::vec3(-1.0f, 9.0f, -5.0f);
    glm::vec3 color = glm::vec3(1.0f);

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
