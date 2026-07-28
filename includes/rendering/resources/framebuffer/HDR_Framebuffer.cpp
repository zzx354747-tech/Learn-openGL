#include "rendering/resources/framebuffer/HDR_Framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
{
        initFramebuffer(width, height);
    }

void Framebuffer::resize(int width, int height)
{
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA16F,
        width, height, 0,
        GL_RGBA, GL_FLOAT,
        nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, 
                GL_DEPTH24_STENCIL8, 
                width, 
                height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
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
        if (index != 0)
        {
            std::cerr << "ERROR::FRAMEBUFFER:: index out of range!" << std::endl;
            return 0;
        }
        return colorBuffer;
    }

unsigned int Framebuffer::getFBO() const
{
        return hdrFBO;
    }

Framebuffer::~Framebuffer()
{
        glDeleteFramebuffers(1, &hdrFBO);
        if (colorBuffer != 0)
            glDeleteTextures(1, &colorBuffer);
        glDeleteRenderbuffers(1, &rboDepth);
    }

void Framebuffer::initFramebuffer(int bfwidth, int bfheight)
{
            // 生成帧缓冲对象
            glGenFramebuffers(1, &hdrFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

            // 生成场景颜色纹理
            glGenTextures(1, &colorBuffer);
            glBindTexture(GL_TEXTURE_2D, colorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA16F,
            bfwidth, bfheight, 0,
            GL_RGBA, GL_FLOAT,
            nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                colorBuffer,
                0);

            // 开启深度测试后，openGL会自动把每个片段的深度值写入
            // 生成渲染缓冲对象
            glGenRenderbuffers(1, &rboDepth);
            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, 
                GL_DEPTH24_STENCIL8, 
                bfwidth, 
                bfheight);

            // 把渲染缓冲对象附加到帧缓冲对象上
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, 
                GL_DEPTH_STENCIL_ATTACHMENT, 
                GL_RENDERBUFFER, 
                rboDepth);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            // 检验帧缓冲对象是否完整
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
