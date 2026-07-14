#include "rendering/passes/ssao/SSAOCommonPass.h"

void SSAOCommonPass::render(int width, int height)
{
    this->width = width;
    this->height = height;
    renderSSAOPass();
    renderSSAOBlurPass();

    resources.registry.setTexture(
        resources.lightingHandles.ao,
        ssao.getBilateralBlurTexture()
    );
}

void SSAOCommonPass::renderSSAOPass()
{
    if (!resources.shaderLibrary)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, ssao.getSSAOFBO());
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    setupSSAOUniforms(resources.shaderLibrary->ssao);
    screenQuad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOCommonPass::renderSSAOBlurPass()
{
    if (!resources.shaderLibrary)
        return;

    Shader& shader = resources.shaderLibrary->ssaoBlur;
    SSAOPingPongFramebuffer& pingPong = ssao.getBilateralBlurPingPong();

    glDisable(GL_DEPTH_TEST);

    shader.use();
    shader.setMat4("view", camera.GetViewMatrix());

    // X pass: 原始 SSAO -> pingPong[0]
    pingPong.bind(0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.setVec2("u_Direction", glm::vec2(1.0f, 0.0f));
    bindSSAOBilateralBlurTextures(shader, ssao.getSSAOColorBuffer());
    screenQuad.draw();

    // Y pass: pingPong[0] -> pingPong[1]，后者是最终 AO
    pingPong.bind(1);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.setVec2("u_Direction", glm::vec2(0.0f, 1.0f));
    bindSSAOBilateralBlurTextures(shader, pingPong.getTextureID(0));
    screenQuad.draw();

    pingPong.unbind();
}

void SSAOCommonPass::setupSSAOUniforms(Shader& shader)
{
    shader.use();
    bindGBufferTextures(shader);
    bindNoiseTexture(shader);

    SSAOCameraUniformSetter::apply(shader, camera, width, height);

    // 将采样核传递给着色器
    for (unsigned int i = 0; i < ssao.getSSAOKernel().size(); ++i)
    {
        shader.setVec3("samples[" + std::to_string(i) + "]", ssao.getSSAOKernel()[i]);
    }
}

void SSAOCommonPass::bindGBufferTextures(Shader& shader)
{
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("gPosition", 0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getPositionTexture());

    glActiveTexture(GL_TEXTURE1);
    shader.setInt("gNormal", 1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getNormalRoughnessTexture());
}

void SSAOCommonPass::bindSSAOBilateralBlurTextures(
    Shader& shader,
    unsigned int aoInput)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, aoInput);
    shader.setInt("u_AOInput", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getPositionTexture());
    shader.setInt("gPosition", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.getNormalRoughnessTexture());
    shader.setInt("gNormalRoughness", 2);
}

void SSAOCommonPass::bindNoiseTexture(Shader& shader)
{
    glActiveTexture(GL_TEXTURE3);
    shader.setInt("texNoise", 3);
    glBindTexture(GL_TEXTURE_2D, ssao.getNoiseTexture());
}

SSAOCommonPass::SSAOCommonPass( SceneRenderResources& resources, SSAO& ssao, Screenquad& screenQuad, Camera& camera, GBuffer& gBuffer ) : resources(resources), ssao(ssao), screenQuad(screenQuad), camera(camera), gBuffer(gBuffer)
{}
