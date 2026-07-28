#include "rendering/passes/blur/BlurPass.h"

void BlurPass::render(
    GLuint inputTexture,
    PingPongFramebuffer& pingpong,
    Shader& blurShader,
    Screenquad& screenQuad,
    int amount
)
{
    bool horizontal = true;
    bool firstIteration = true;

    glDisable(GL_DEPTH_TEST);

    blurShader.use();
    blurShader.setInt("image", 0);

    for (int i = 0; i < amount; ++i)
    {
        // horizontal == true  写入 pingpong[0]
        // horizontal == false 写入 pingpong[1]
        pingpong.bind(horizontal ? 0 : 1);

        blurShader.setBool("horizontal", horizontal);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            firstIteration ? inputTexture
                : pingpong.getTextureID(horizontal ? 1 : 0)
        );

        screenQuad.draw();

        horizontal = !horizontal;
        firstIteration = false;
    }

    pingpong.unbind();
}
