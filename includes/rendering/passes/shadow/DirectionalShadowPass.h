#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/core/ResourceRegistry.h"
#include "scene/Camera.h"

class GpuProfiler;

class DirectionalShadowPass
{
public:
    DirectionalShadowPass(DirectionalShadowMap& shadowMap,
        Shader& shadowShader,
        Shader& cloudOpticalDepthShader,
        Shader& cloudOpticalDepthBlurShader,
        Shader& cloudOpticalDepthToTransmittanceShader,
        SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
        Camera& camera,
        SceneRenderState& state, LightSettings& lightSettings,
        SceneRenderConfig& config,
        ResourceRegistry& registry, ResourceHandle shadowMapHandle);
    ~DirectionalShadowPass();

    void render(Screenquad& screenQuad, GpuProfiler& profiler);
    unsigned int cloudOpticalDepthTexture() const
    {
        return filteredCloudOpticalDepthTexture;
    }
    unsigned int cloudTransmittanceTexture() const
    {
        return cloudTransmittanceTextureId;
    }

private:
    ResourceRegistry& registry;
    ResourceHandle shadowMapHandle;
    DirectionalShadowMap& shadowMap;
    Shader& shadowShader;
    Shader& cloudOpticalDepthShader;
    Shader& cloudOpticalDepthBlurShader;
    Shader& cloudOpticalDepthToTransmittanceShader;
    SphereDrawer& sphereDrawer;
    ModelDrawer& modelDrawer;
    Camera& camera;
    SceneRenderState& state;
    LightSettings& lightSettings;
    SceneRenderConfig& config;
    unsigned int rawCloudOpticalDepthTexture = 0;
    unsigned int filteredCloudOpticalDepthTexture = 0;
    unsigned int cloudTransmittanceTextureId = 0;
    unsigned int rawCloudFramebuffer = 0;
    unsigned int filteredCloudFramebuffer = 0;
    unsigned int cloudTransmittanceFramebuffer = 0;
    unsigned int cloudScanSlice = 0;
    glm::vec2 previousCloudOriginLS = glm::vec2(0.0f);
    glm::vec3 previousTowardSun = glm::vec3(0.0f, 1.0f, 0.0f);
    float previousWeatherCoverage = -1.0f;
    float previousMapCoverage = -1.0f;
    float previousDensity = -1.0f;
    float previousCloudBaseHeight = -1.0f;
    float previousCloudThickness = -1.0f;
    float previousHoleStrength = -1.0f;
    unsigned int previousHoleSeed = 0;
    int previousHoleCount = -1;
    bool previousCloudEnabled = false;
    bool cloudTextureInitialized = false;
    static constexpr int CloudShadowResolution = 512;

    glm::mat4 createLightSpaceMatrix() const;
    glm::mat4 createCloudLightSpaceMatrix(glm::vec2& snappedOriginLS) const;
    void createCloudTextures();
    bool cloudShadowNeedsFullUpdate(
        const glm::vec2& snappedOriginLS,
        const glm::vec3& towardSun) const;
    void renderCloudOpticalDepth(
        Screenquad& screenQuad, GpuProfiler& profiler);
    void renderCloudIntegration(
        Screenquad& screenQuad, int rowOffset, int rowCount);
    void renderCloudBlur(
        Screenquad& screenQuad, int rowOffset, int rowCount);
    void renderCloudTransmittance(
        Screenquad& screenQuad, int rowOffset, int rowCount);
    void setCloudDensityUniforms(Shader& shader) const;
};
