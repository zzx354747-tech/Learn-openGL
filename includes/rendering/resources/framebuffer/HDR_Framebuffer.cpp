#include "rendering/resources/framebuffer/HDR_Framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
{
        initFramebuffer(width, height);
    }

void Framebuffer::resize(int width, int height)
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

        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                     width, height, 0, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

void Framebuffer::bind()
{
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    }

void Framebuffer::unbind()
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

unsigned int Framebuffer::getTextureID(int index) const
{
        if (index < 0 || index >= 2)
        {
            std::cerr << "ERROR::FRAMEBUFFER:: index out of range!" << std::endl;
            return 0;
        }
        return colorBuffers[index];
    }

unsigned int Framebuffer::getFBO() const
{
        return hdrFBO;
    }

unsigned int Framebuffer::getDepthTextureID() const
{
        return depthTexture;
    }

Framebuffer::~Framebuffer()
{
        glDeleteFramebuffers(1, &hdrFBO);
        for (int i = 0; i < 2; ++i)
        {
            if (colorBuffers[i] != 0)
                glDeleteTextures(1, &colorBuffers[i]);
        }
        glDeleteTextures(1, &depthTexture);
    }

void Framebuffer::initFramebuffer(int bfwidth, int bfheight)
{
            // 生成帧缓冲对象
            glGenFramebuffers(1, &hdrFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

            // 生成纹理
            for (int i = 0; i < 2; ++i)
            {
                glGenTextures(1, &colorBuffers[i]);
                glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
                // 创建一个这帧所包含的所有画面的纹理
                glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGBA16F,
                bfwidth, bfheight, 0,
                GL_RGBA, GL_FLOAT,  // 匹配内部格式
                nullptr);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // 把纹理附加到帧缓冲对象上
                glFramebufferTexture2D(GL_FRAMEBUFFER, 
                    GL_COLOR_ATTACHMENT0 + i,
                    GL_TEXTURE_2D, 
                    colorBuffers[i], 
                    0);
            }

            // 开启深度测试后，openGL会自动把每个片段的深度值写入
            // 生成渲染缓冲对象
            glGenTextures(1, &depthTexture);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                         bfwidth, bfheight, 0, GL_DEPTH_STENCIL,
                         GL_UNSIGNED_INT_24_8, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // 把渲染缓冲对象附加到帧缓冲对象上
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_TEXTURE_2D,
                depthTexture,
                0);

            // 显示声明调用drawbuffers,以及声明drawbuffer列表
            GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);

            // 检验帧缓冲对象是否完整
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
