#pragma once

#include <glad/gl.h>
#include <iostream>

class GBuffer
{
public:
    GBuffer(int width, int height)
    {
        initGBuffer(width, height);
    }

    void resize(int width, int height)
    {
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);

        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    }

    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint getFBO() const 
    {
        return gBuffer;
    }

    unsigned int getGbufferTextureID(int index) const
    {
        if (index < 0 || index >= 3)
        {
            std::cerr << "ERROR::GBUFFER:: index out of range!" << std::endl;
            return 0;
        }
        return attachments[index];
    }

    ~GBuffer()
    {
        glDeleteFramebuffers(1, &gBuffer);
        glDeleteTextures(1, &gPosition);
        glDeleteTextures(1, &gNormal);
        glDeleteTextures(1, &gAlbedoSpec);
        glDeleteRenderbuffers(1, &rboDepth);
    }

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

private:
    GLuint gBuffer = 0;
    GLuint attachments[3] = { 0, 0, 0 };
    GLuint gPosition = 0;
    GLuint gNormal = 0;
    GLuint gAlbedoSpec = 0;
    GLuint rboDepth = 0;

    void initGBuffer(int width, int height)
    {
        // 生成帧缓冲对象
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

        // 位置纹理
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

        attachments[0] = gPosition;

        // 法线纹理
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

        attachments[1] = gNormal;

        // 漫反射/镜面反射纹理
        glGenTextures(1, &gAlbedoSpec);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

        attachments[2] = gAlbedoSpec;

        // 设置要渲染的附件
        GLenum drawbuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, drawbuffers);

        // 深度缓冲区
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        // 检查帧缓冲是否完整
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "ERROR::GBUFFER:: Framebuffer is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};