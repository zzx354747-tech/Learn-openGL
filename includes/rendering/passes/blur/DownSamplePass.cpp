#include "rendering/passes/blur/DownSamplePass.h"

DownSamplePass::DownSamplePass(int width, int height,
    Shader& shader, Screenquad& quad)
    : downSampleShader(shader),
    screenQuad(quad),
    downSampleTexture(width, height,
                      GL_RGB16F,        // internalFormat
                      GL_RGB,           // format
                      GL_FLOAT,         // type  ← 改对了,对应 GL_RGB16F
                      GL_CLAMP_TO_EDGE, // wrapMode
                      GL_LINEAR)        // filterMode
{
    initResources();
}

DownSamplePass::~DownSamplePass()
{
    glDeleteFramebuffers(1, &downSampleFBO);
}

void DownSamplePass::initResources()
{
    // 只创建/绑定/附加 FBO,纹理已经在初始化列表(或 resize 的移动赋值)里就绪
    glGenFramebuffers(1, &downSampleFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, downSampleFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, downSampleTexture.getID(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("DownSamplePass: Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DownSamplePass::render(GLuint inputTexture)
{
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, downSampleFBO);
    glViewport(0, 0, downSampleTexture.getWidth(), downSampleTexture.getHeight());

    // 关闭深度测试
    glDisable(GL_DEPTH_TEST);

    downSampleShader.use();
    // sceneColor从纹理单元0读取
    downSampleShader.setInt("sceneColor", 0);

    glActiveTexture(GL_TEXTURE0);
    // inputTexture为纹理对象id
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    // Render a full-screen quad
    screenQuad.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (depthTestWasEnabled == GL_TRUE)
    {
        glEnable(GL_DEPTH_TEST);
    }
}

void DownSamplePass::resize(int width, int height)
{
    GLTexture newTexture(width, height,
                         GL_RGB16F,        // internalFormat
                         GL_RGB,           // format
                         GL_FLOAT,         // type  ← 改对了,对应 GL_RGB16F
                         GL_CLAMP_TO_EDGE, // wrapMode
                         GL_LINEAR);       // filterMode

    downSampleTexture = std::move(newTexture);

    // 重新附加纹理到FBO
    glBindFramebuffer(GL_FRAMEBUFFER, downSampleFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, downSampleTexture.getID(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("DownSamplePass: Framebuffer is not complete after resize!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
