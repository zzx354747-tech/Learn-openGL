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
    bool enableFloor = false;
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
    bool enableClearSphere = true;
    bool enableModel = true;
    float ssaoStrength = 1.0f;
    glm::vec3 fixedAmbientColor = glm::vec3(0.08f);
    float fixedAmbientStrength = 1.0f;
    glm::vec3 iblAmbientTint = glm::vec3(1.0f);
    float iblAmbientStrength = 1.0f;

    // Cube
    bool cubeEnableNormalMapping = false;
    bool cubeEnableParallaxMapping = false;
    float cubeParallaxHeightScale = 0.03f;
    int cubeNumLayers = 32;

    // Floor
    bool floorEnableNormalMapping = true;
    bool floorEnableParallaxMapping = false;
    float floorParallaxHeightScale = 0.03f;
    float floorBumpNormalStrength = 1.0f;
    int floorNumLayers = 32;

    // Model
    bool modelEnableNormalMapping = false;
    bool modelEnableParallaxMapping = false;
    float modelParallaxHeightScale = 0.03f;
    float modelBumpNormalStrength = 2.0f;
    int modelNumLayers = 32;

    int numBlurPasses = 10;
    float exposure = 1.0f;
    float bloomStrength = 0.6f;
    float bloomThreshold = 0.7f;
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

    glm::vec3 clearSpherePosition = glm::vec3(0.0f, 0.55f, -4.8f);

    glm::vec3 lightPositions = glm::vec3(-1.0f, 9.0f, -5.0f);
    glm::vec3 color = glm::vec3(1.0f);

    glm::mat4 dirLightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 spotLightSpaceMatrix = glm::mat4(1.0f);
};
