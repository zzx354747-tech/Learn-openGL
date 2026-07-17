#pragma once

#include <glm/glm.hpp>

#include "rendering/resources/render/SceneRenderInputResources.h"
#include "rendering/resources/render/SceneRenderOutputResources.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/resources/shadow/PointShadowMap.h"
#include "rendering/resources/shadow/SpotShadowMap.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"

#include "rendering/core/ResourceRegistry.h"
#include "rendering/passes/lighting/LightingPassResourceHandles.h"
#include "rendering/resources/shader/ShaderLibrary.h"

class AlpineVegetationSystem;

struct SceneRenderResources : SceneRenderInputResources, SceneRenderOutputResources
{
    ResourceRegistry registry;
    LightingPassResourceHandles lightingHandles;
    ShaderLibrary* shaderLibrary = nullptr;
    AlpineVegetationSystem* vegetationSystem = nullptr;
    unsigned int cloudAccelerationTexture = 0;
    unsigned int cloudSunLocalTexture = 0;
    unsigned int cloudOpticalDepthTexture = 0;
    unsigned int cloudTransmittanceTexture = 0;
    bool cloudAccelerationValid = false;
    bool cloudSunLocalValid = false;
    glm::vec2 cloudCacheOrigin = glm::vec2(0.0f);
    glm::vec2 cloudSunLocalOrigin = glm::vec2(0.0f);
    float cloudCacheWorldSize = 1.0f;
    float cloudSunLocalWorldSize = 1.0f;

    SceneRenderResources();

    void declareLightingPassResources();
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
    unsigned int cloudOpticalDepthTexture = 0;
    unsigned int cloudTransmittanceTexture = 0;
    PointShadowMap* pointShadowMap = nullptr;
    SpotShadowMap* spotShadowMap = nullptr;
};
