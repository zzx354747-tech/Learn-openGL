#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/postprocess/Gbuffer.h"
#include "rendering/uniforms/ShadowMapBinder.h"
#include "rendering/postprocess/HDR_Framebuffer.h"
#include "rendering/uniforms/PointShadowUniformSetters.h"
#include "rendering/uniforms/SpotShadowUniformSetter.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/postprocess/SSAO.h"

class LightingPass
{
public:
    LightingPass(
        SceneRenderResources& resources,
        ShadowResources& shadowResources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        LightSettings& lightSettings,
        Camera& camera,
        GBuffer& gBuffer
    ) : resources(resources),
        shadowResources(shadowResources),
        config(config),
        state(state),
        lightSettings(lightSettings),
        camera(camera),
        gBuffer(gBuffer)
    {}

    void render(Framebuffer& framebuffer,
        Screenquad& screenQuad,
        unsigned int aoTexture);

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    LightSettings& lightSettings;
    Camera& camera;
    GBuffer& gBuffer;

    void setupObjectLighting(Shader& shader);
    // 设置第二次渲染时的点光源阴影贴图和相关uniform
    void setupPointShadow(Shader& shader, unsigned int textureUnit = 11);
    void setupSpotShadow(Shader& shader, unsigned int textureUnit = 12);

    Shader* getLightingShader();

    void bindGBufferTextures(Shader& shader, unsigned int aoTexture);
};