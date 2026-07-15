#include "rendering/resources/framebuffer/TemporalAAFramebuffer.h"

#include <iostream>

TemporalAAFramebuffer::TemporalAAFramebuffer(int width, int height)
{
    init(width, height);
}

void TemporalAAFramebuffer::init(int width, int height)
{
    glGenFramebuffers(2, fbos_);
    glGenTextures(2, textures_);

    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbos_[i]);
        glBindTexture(GL_TEXTURE_2D, textures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, textures_[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR::TAA:: history framebuffer is incomplete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TemporalAAFramebuffer::resize(int width, int height)
{
    for (GLuint texture : textures_)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TemporalAAFramebuffer::bindWrite(int index) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbos_[index & 1]);
}

GLuint TemporalAAFramebuffer::getTextureID(int index) const
{
    return textures_[index & 1];
}

TemporalAAFramebuffer::~TemporalAAFramebuffer()
{
    glDeleteFramebuffers(2, fbos_);
    glDeleteTextures(2, textures_);
}
