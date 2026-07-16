#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/uniforms/LightUniformSetter.h"

#include <array>

class LightingPass
{
public:
    struct CausticMapStats
    {
        float maximumDynamicDensity = 0.0f;
        float maximumReferenceDensity = 0.0f;
        float maximumFocusedExcess = 0.0f;
        bool valid = false;
    };

    LightingPass( SceneRenderResources& resources, ShadowResources& shadowResources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, Camera& camera );
    ~LightingPass();

    void render(Framebuffer& framebuffer,
        Screenquad& screenQuad);
    CausticMapStats inspectCausticMap() const;

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    LightSettings& lightSettings;
    Camera& camera;
    std::array<unsigned int, 2> causticFramebuffers{{0u, 0u}};
    std::array<unsigned int, 2> causticTextures{{0u, 0u}};
    std::array<glm::vec4, 2> causticBounds{{glm::vec4(0.0f), glm::vec4(0.0f)}};
    unsigned int photonVAO = 0;
    unsigned int photonVBO = 0;
    int photonVertexCount = 0;
    float currentWaterTime = 0.0f;
    bool causticResourcesReady = false;

    void setupObjectLighting(Shader& shader);

    Shader* getLightingShader();

    void setupPointShadowUniform(Shader& shader);
    void setupSpotShadowUniform(Shader& shader);

    void bindLightingInputTextures(Shader& shader);
    void initializeCausticResources();
    void renderCausticMap(Screenquad& screenQuad);
};
