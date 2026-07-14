#include "rendering/resources/framebuffer/SSAOPingPongFramebuffer.h"

SSAOPingPongFramebuffer::SSAOPingPongFramebuffer(int width, int height)
{
    initFramebuffer(width, height);
}

void SSAOPingPongFramebuffer::initFramebuffer(int width, int height)
{
    glGenFramebuffers(2, fbos);
    glGenTextures(2, colorBuffers);

    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);

        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R16F,
            width,
            height,
            0,
            GL_RED,
            GL_FLOAT,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorBuffers[i],
            0
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr
                << "ERROR::SSAO_PINGPONG:: Framebuffer "
                << i
                << " is not complete!"
                << std::endl;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPingPongFramebuffer::resize(int width, int height)
{
    for (GLuint colorBuffer : colorBuffers)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R16F,
            width,
            height,
            0,
            GL_RED,
            GL_FLOAT,
            nullptr
        );
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void SSAOPingPongFramebuffer::bind(int index)
{
    if (index < 0 || index >= 2)
    {
        std::cerr << "ERROR::SSAO_PINGPONG:: index out of range!" << std::endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbos[index]);
}

void SSAOPingPongFramebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint SSAOPingPongFramebuffer::getTextureID(int index) const
{
    if (index < 0 || index >= 2)
    {
        std::cerr << "ERROR::SSAO_PINGPONG:: index out of range!" << std::endl;
        return 0;
    }

    return colorBuffers[index];
}

SSAOPingPongFramebuffer::~SSAOPingPongFramebuffer()
{
    glDeleteFramebuffers(2, fbos);
    glDeleteTextures(2, colorBuffers);
}
