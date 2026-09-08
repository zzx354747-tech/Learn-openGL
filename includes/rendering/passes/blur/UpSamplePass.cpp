#include "UpSamplePass.h"

UpsamplePass::UpsamplePass(Shader& shader, Screenquad& quad)
    : upsampleShader(shader),
      screenQuad(quad)
{
}

void UpsamplePass::render(int width, int height,
    GLuint sourceTexture, GLuint targetFBO, float filterRadius)
{
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, width, height);

    upsampleShader.use();
    upsampleShader.setInt("uSourceTexture", 0);
    upsampleShader.setFloat("uFilterRadius", filterRadius);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);

    screenQuad.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (depthTestWasEnabled == GL_TRUE)
    {
        glEnable(GL_DEPTH_TEST);
    }
}
