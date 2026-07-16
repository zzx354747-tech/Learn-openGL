#include "rendering/postprocess/screen/ScreenPass.h"

void ScreenPass::render(
    int bfwidth,
    int bfheight,
    Shader& screenShader,
    Screenquad& screenQuad,
    Framebuffer& framebuffer,
    GLuint sceneTexture
)
{
    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(config.skyTopColor.r, config.skyTopColor.g, config.skyTopColor.b, 1.0f);
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
    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    glActiveTexture(GL_TEXTURE1);
    if (config.enableBloom && resources.pingpongFBO)
    {
        glBindTexture(
            GL_TEXTURE_2D,
            resources.pingpongFBO->getTextureID(1)
        );
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(0));
    }

    screenQuad.draw();
}

ScreenPass::ScreenPass(SceneRenderConfig& config, SceneRenderResources& resources,
                       Camera& camera, LightSettings& lightSettings)
    : config(config), resources(resources), camera(camera), lightSettings(lightSettings)
{
    }
