#include "rendering/passes/screen/ScreenPass.h"

void ScreenPass::render(
    int bfwidth,
    int bfheight,
    Shader& screenShader,
    Screenquad& screenQuad,
    Framebuffer& framebuffer
)
{
    // Present to the window; never sample the HDR texture while drawing into it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    screenShader.use();

    screenShader.setInt("screenTexture", 0);
    screenShader.setInt("bloomBlur", 1);

    screenShader.setFloat("exposure", config.exposure);
    screenShader.setBool("enableHdr", config.enableHDR);
    screenShader.setBool("enableGamma", config.enableGammaCorrection);
    screenShader.setBool("enableBloom", config.enableBloom);
    screenShader.setFloat("bloomStrength", config.bloomStrength);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(0));

    glActiveTexture(GL_TEXTURE1);
    if (config.enableBloom)
    {
        glBindTexture(
            GL_TEXTURE_2D,
            bloomPass.GetBloomTextureID()
        );
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(0));
    }

    screenQuad.draw();
}

ScreenPass::ScreenPass(SceneRenderConfig& config, BloomPass& bloomPass)
    : config(config), bloomPass(bloomPass)
{
}
