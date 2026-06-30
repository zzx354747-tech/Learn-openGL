#pragma once

#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/render/SceneRenderTypes.h"

class LightUniformSetter
{
public:
    static void apply( Shader& shader, const LightSettings& lightSettings, const SceneRenderConfig& config, const SceneRenderState& state, const Camera& camera );

private:
    static void setupPointLight( Shader& shader, const SceneRenderState& state, const LightSettings& lightSettings );

    static void setupDirectionalLight(Shader& shader, const LightSettings& lightSettings);

    static void setupFlashLight( Shader& shader, const Camera& camera, const LightSettings& lightSettings );

    static glm::vec3 getFlashLightPosition( const Camera& camera, const LightSettings& lightSettings );
};
