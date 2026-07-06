#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/shadow/SpotShadowMap.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/core/ModelDrawer.h" 
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/core/ResourceRegistry.h"

class SpotShadowPass
{
public:
    SpotShadowPass( SpotShadowMap& spotShadowMap, Shader& spotShadowShader, 
        SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
        Camera& camera, SceneRenderState& state, 
        LightSettings& lightSettings, ResourceRegistry& registry,
         ResourceHandle spotShadowMapHandle);

    void render();

private:
    ResourceRegistry& registry;
    ResourceHandle spotShadowMapHandle;
    SpotShadowMap& spotShadowMap;
    Shader& spotShadowShader;
    SphereDrawer& sphereDrawer;
    ModelDrawer& modelDrawer;
    Camera& camera;
    SceneRenderState& state;
    LightSettings& lightSettings;

    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

    glm::mat4 createLightSpaceMatrix();

    glm::vec3 getFlashLightPosition() const;
};
