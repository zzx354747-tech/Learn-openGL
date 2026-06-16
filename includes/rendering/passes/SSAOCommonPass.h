#pragma once
#include "rendering/postprocess/SSAO.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/uniforms/SSAOCameraUniformSetter.h"
#include "rendering/postprocess/Gbuffer.h"
#include "core/Shader.h"
#include "scene/Camera.h"

class SSAOCommonPass
{
public:
    SSAOCommonPass(
        SceneRenderResources& resources,
        SSAO& ssao,
        Screenquad& screenQuad,
        Camera& camera,
        GBuffer& gBuffer
    ) : resources(resources),
        ssao(ssao),
        screenQuad(screenQuad),
        camera(camera),
        gBuffer(gBuffer)
    {}
    void render(int width, int height);
    void renderSSAOPass();
    void renderSSAOBlurPass();

private:
    SceneRenderResources& resources;
    SSAO& ssao;
    Screenquad& screenQuad;
    Camera& camera;
    GBuffer& gBuffer;
    float width = 0.0f;
    float height = 0.0f;
    void setupSSAOUniforms(Shader& shader);
    void bindGBufferTextures(Shader& shader);
    void bindSSAOInputTexture(Shader& shader);
    void bindNoiseTexture(Shader& shader);
};
