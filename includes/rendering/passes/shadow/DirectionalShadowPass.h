#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/core/ResourceRegistry.h"

class DirectionalShadowPass
{
public:
    DirectionalShadowPass( DirectionalShadowMap& shadowMap, Shader& shadowShader, 
        SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
        SceneRenderState& state, LightSettings& lightSettings,
        SceneRenderConfig& config,
        ResourceRegistry& registry, ResourceHandle shadowMapHandle);

    void render();

private:
    ResourceRegistry& registry;
    ResourceHandle shadowMapHandle;
    DirectionalShadowMap& shadowMap;
    Shader& shadowShader;
    SphereDrawer& sphereDrawer;
    ModelDrawer& modelDrawer;
    SceneRenderState& state;
    LightSettings& lightSettings;
    SceneRenderConfig& config;

    glm::mat4 createLightSpaceMatrix() const;
};
