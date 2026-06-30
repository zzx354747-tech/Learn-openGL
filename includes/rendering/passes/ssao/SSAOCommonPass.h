#pragma once
#include "rendering/resources/ssao/SSAO.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/uniforms/SSAOCameraUniformSetter.h"
#include "rendering/resources/framebuffer/Gbuffer.h"
#include "core/Shader.h"
#include "scene/Camera.h"

class SSAOCommonPass
{
public:
    SSAOCommonPass( SceneRenderResources& resources, SSAO& ssao, Screenquad& screenQuad, Camera& camera, GBuffer& gBuffer );
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
