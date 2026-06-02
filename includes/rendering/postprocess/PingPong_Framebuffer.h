#pragma once 

#include <glad/gl.h>
#include <iostream>

class PingPongFramebuffer
{
public:
    PingPongFramebuffer(int width, int height)
    {
        initFramebuffer(width, height);
    }

    void resize(int width, int height)
    {
        for (int i = 0; i < 2; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            // 创建一个这帧所包含的所有画面的纹理
            glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA16F,
            width, height, 0,
            GL_RGBA, GL_FLOAT,  // 匹配内部格式
            nullptr);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void bind(int index)
    {
        if (index < 0 || index >= 2)
        {
            std::cerr << "ERROR::PINGPONG_FRAMEBUFFER:: index out of range!" << std::endl;
            return;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, fbos[index]);
    }

    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint getTextureID(int index) const
    {
        if (index < 0 || index >= 2)
        {
            std::cerr << "ERROR::PINGPONG_FRAMEBUFFER:: index out of range!" << std::endl;
            return 0;
        }
        return colorBuffers[index];
    }

    ~PingPongFramebuffer()
    {
        glDeleteFramebuffers(2, fbos);
        glDeleteTextures(2, colorBuffers);
    }

private:
    GLuint fbos[2] = { 0, 0 };
    GLuint colorBuffers[2] = { 0, 0 };

    void initFramebuffer(int width, int height)
    {
        glGenFramebuffers(2, fbos);
        glGenTextures(2, colorBuffers);

        for (int i = 0; i < 2; ++i)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGBA16F,
                width, height, 0,
                GL_RGBA, GL_FLOAT,
                nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                colorBuffers[i],
                0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "ERROR::PINGPONG_FRAMEBUFFER:: Framebuffer " << i << " is not complete!" << std::endl;
            }
            
             glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};