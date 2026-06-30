#include "rendering/resources/shadow/DirectionalShadowMap.h"

DirectionalShadowMap::DirectionalShadowMap( int width, int height) : width(width), height(height)
{
        initShadowMap();
    }

unsigned int DirectionalShadowMap::getDepthMapTexture() const
{
        return depthMap;
    }

unsigned int DirectionalShadowMap::getFBO() const
{ return depthMapFBO; }

int DirectionalShadowMap::getWidth() const
{ return width; }

int DirectionalShadowMap::getHeight() const
{ return height; }

DirectionalShadowMap::~DirectionalShadowMap()
{
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthMap);
    }

void DirectionalShadowMap::initShadowMap()
{
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        // 创建一个深度纹理来存储从光源视角看到的深度信息
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        // 给超出光源采样的范围，返回最远深度值
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // 将深度纹理附加到帧缓冲对象上
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Directional Shadow Map Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
