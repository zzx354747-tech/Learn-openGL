#pragma once
#include <glad/gl.h>
#include <iostream>

class Framebuffer
{
public:
    Framebuffer(int width, int height)
    {
        initFramebuffer(width, height);
    }

    void resize(int width, int height)
    {
        glBindTexture(GL_TEXTURE_2D, texColorBuffer);
        // 创建一个这帧所包含的所有画面的纹理
        glTexImage2D(GL_TEXTURE_2D, 
            0, 
            GL_RGB, 
            width, 
            height, 
            0, 
            GL_RGB, 
            GL_UNSIGNED_BYTE, 
            nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, 
            GL_DEPTH24_STENCIL8, 
            width, 
            height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    unsigned int getTextureID() const
    {
        return texColorBuffer;
    }

    ~Framebuffer()
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texColorBuffer);
        glDeleteRenderbuffers(1, &rbo);
    }


    private:
        unsigned int texColorBuffer = 0;
        unsigned int rbo = 0;
        unsigned int fbo = 0;

        void initFramebuffer(int bfwidth, int bfheight)
        {
            // 生成帧缓冲对象
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);

            // 生成纹理
            glGenTextures(1, &texColorBuffer);
            glBindTexture(GL_TEXTURE_2D, texColorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 
                0, 
                GL_RGB, 
                bfwidth, 
                bfheight, 
                0, 
                GL_RGB, 
                GL_UNSIGNED_BYTE, 
                nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // 把纹理附加到帧缓冲对象上
            glFramebufferTexture2D(GL_FRAMEBUFFER, 
                GL_COLOR_ATTACHMENT0, 
                GL_TEXTURE_2D, 
                texColorBuffer, 
                0);

            // 生成渲染缓冲对象
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, 
                GL_DEPTH24_STENCIL8, 
                bfwidth, 
                bfheight);

            // 把渲染缓冲对象附加到帧缓冲对象上
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, 
                GL_DEPTH_STENCIL_ATTACHMENT, 
                GL_RENDERBUFFER, 
                rbo);

            // 检验帧缓冲对象是否完整
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
};