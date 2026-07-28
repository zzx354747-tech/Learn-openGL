#include "BrightPrefilterPass.h"

BrightPrefilterPass::BrightPrefilterPass(int width, int height,
    Shader& shader, Screenquad& quad)
    : brightPrefilterShader(shader),
    screenQuad(quad),
    brightPrefilterTexture(width, height,
                            GL_RGB16F,        // internalFormat
                            GL_RGB,           // format
                            GL_FLOAT,         // type  ← 改对了,对应 GL_RGB16F
                            GL_CLAMP_TO_EDGE, // wrapMode
                            GL_LINEAR)        // filterMode
{
    initResources(width, height);
}

void BrightPrefilterPass::initResources(int width, int height)
{
    // 只创建/绑定/附加 FBO,纹理已经在初始化列表(或 resize 的移动赋值)里就绪
    glGenFramebuffers(1, &BrightPrefilterFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, BrightPrefilterFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, brightPrefilterTexture.getID(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("BrightPrefilterPass: Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BrightPrefilterPass::~BrightPrefilterPass()
{
    glDeleteFramebuffers(1, &BrightPrefilterFBO);
}

void BrightPrefilterPass::render(GLuint inputTexture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, BrightPrefilterFBO);
    glViewport(0, 0, brightPrefilterTexture.getWidth(), brightPrefilterTexture.getHeight());

    brightPrefilterShader.use();
    // 设置阈值
    brightPrefilterShader.setFloat("threshold", threshold);
    // 设置软阈值过渡范围
    brightPrefilterShader.setFloat("knee", knee);
    // sceneColor从纹理单元0读取
    brightPrefilterShader.setInt("sceneColor", 0);

    glActiveTexture(GL_TEXTURE0);
    // inputTexture为纹理对象id
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    // Render a full-screen quad
    screenQuad.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BrightPrefilterPass::resize(int width, int height)
{
    // 重新创建纹理
    GLTexture newTexture(width, height,
                         GL_RGB16F,        // internalFormat
                         GL_RGB,           // format
                         GL_FLOAT,         // type
                         GL_CLAMP_TO_EDGE, // wrapMode
                         GL_LINEAR);       // filterMode

    brightPrefilterTexture = std::move(newTexture);

    // 重新绑定 FBO 附加新的纹理
    glBindFramebuffer(GL_FRAMEBUFFER, BrightPrefilterFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, brightPrefilterTexture.getID(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("BrightPrefilterPass: Framebuffer is not complete after resize!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}