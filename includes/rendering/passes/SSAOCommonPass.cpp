#include "rendering/passes/SSAOCommonPass.h"

void SSAOCommonPass::render(int width, int height)
{
    this->width = static_cast<float>(width);
    this->height = static_cast<float>(height);
    renderSSAOPass();
    renderSSAOBlurPass();
}

void SSAOCommonPass::renderSSAOPass()
{
    if (!resources.ssaoShader)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, ssao.getSSAOFBO());
    glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    setupSSAOUniforms(*resources.ssaoShader);
    screenQuad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOCommonPass::renderSSAOBlurPass()
{
    if (!resources.ssaoBlurShader)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, ssao.getSSAOBlurFBO());
    glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    resources.ssaoBlurShader->use();
    bindSSAOInputTexture(*resources.ssaoBlurShader);
    screenQuad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void SSAOCommonPass::bindSSAOInputTexture(Shader& shader)
{
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("SSAOInput", 0);
    glBindTexture(GL_TEXTURE_2D, ssao.getSSAOColorBuffer());
}

void SSAOCommonPass::bindNoiseTexture(Shader& shader)
{
    glActiveTexture(GL_TEXTURE3);
    shader.setInt("texNoise", 3);
    glBindTexture(GL_TEXTURE_2D, ssao.getNoiseTexture());
}
