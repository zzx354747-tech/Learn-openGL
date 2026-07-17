#include "rendering/resources/framebuffer/Gbuffer.h"

GBuffer::GBuffer(int width, int height)
{
        initGBuffer(width, height);
    }

void GBuffer::resize(int width, int height)
{
        glBindTexture(GL_TEXTURE_2D, gPosition);
        // Absolute world-space positions require full precision across the
        // kilometre-sized terrain. RGB16F quantizes them into visible cells.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);

        glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glBindTexture(GL_TEXTURE_2D, gAlbedoMetallic);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, gVelocity);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

void GBuffer::bind()
{
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    }

void GBuffer::unbind()
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

GLuint GBuffer::getFBO() const
{
        return gBuffer;
    }

unsigned int GBuffer::getGbufferTextureID(int index) const
{
        if (index < 0 || index >= 4)
        {
            std::cerr << "ERROR::GBUFFER:: index out of range!" << std::endl;
            return 0;
        }
        return attachments[index];
    }

unsigned int GBuffer::getPositionTexture() const
{ return gPosition; }

unsigned int GBuffer::getNormalRoughnessTexture() const
{ return gNormalRoughness; }

unsigned int GBuffer::getAlbedoMetallicTexture() const
{ return gAlbedoMetallic; }

unsigned int GBuffer::getVelocityTexture() const
{ return gVelocity; }

void GBuffer::blitDepthTo(Framebuffer& framebuffer, int width, int height)
{
        glBindFramebuffer(GL_READ_FRAMEBUFFER, getFBO());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.getFBO());
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

GBuffer::~GBuffer()
{
        glDeleteFramebuffers(1, &gBuffer);
        glDeleteTextures(1, &gPosition);
        glDeleteTextures(1, &gNormalRoughness);
        glDeleteTextures(1, &gAlbedoMetallic);
        glDeleteTextures(1, &gVelocity);
        glDeleteRenderbuffers(1, &rboDepth);
    }

void GBuffer::initGBuffer(int width, int height)
{
        // 生成帧缓冲对象
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

        // 位置纹理
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        // Keep the initialization path consistent with resize().
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

        attachments[0] = gPosition;

        // 法线/粗糙度纹理
        glGenTextures(1, &gNormalRoughness);
        glBindTexture(GL_TEXTURE_2D, gNormalRoughness);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalRoughness, 0);

        attachments[1] = gNormalRoughness;

        // 漫反射/金属度/反射率纹理
        glGenTextures(1, &gAlbedoMetallic);
        glBindTexture(GL_TEXTURE_2D, gAlbedoMetallic);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoMetallic, 0);

        attachments[2] = gAlbedoMetallic;

        // Dynamic geometry stores previous-minus-current UV displacement.
        // Static geometry writes zero, so camera reprojection remains owned by TAA.
        glGenTextures(1, &gVelocity);
        glBindTexture(GL_TEXTURE_2D, gVelocity);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gVelocity, 0);
        attachments[3] = gVelocity;

        // 设置要渲染的附件
        GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                  GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, drawBuffers);
        
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
