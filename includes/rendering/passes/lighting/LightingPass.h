#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/uniforms/LightUniformSetter.h"

class LightingPass
{
public:
    LightingPass( SceneRenderResources& resources, ShadowResources& shadowResources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, Camera& camera );

    void render(Framebuffer& framebuffer,
        Screenquad& screenQuad);

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    LightSettings& lightSettings;
    Camera& camera;

    void setupObjectLighting(Shader& shader);

    Shader* getLightingShader();

    void setupPointShadowUniform(Shader& shader);
    void setupSpotShadowUniform(Shader& shader);

    void bindLightingInputTextures(Shader& shader);
};
