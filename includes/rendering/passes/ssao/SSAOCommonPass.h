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
    int width = 0;
    int height = 0;
    void setupSSAOUniforms(Shader& shader);
    void bindGBufferTextures(Shader& shader);
    void bindSSAOBilateralBlurTextures(Shader& shader, unsigned int aoInput);
    void bindNoiseTexture(Shader& shader);
};
