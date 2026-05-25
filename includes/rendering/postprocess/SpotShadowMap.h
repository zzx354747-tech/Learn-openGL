#pragma once

#include <glad/gl.h>
#include <iostream>

class SpotShadowMap
{
public:
    SpotShadowMap(int width, 
        int height,
        float nearPlane,
        float farPlane)
        : width(width), 
        height(height),
        nearPlane(nearPlane),
        farPlane(farPlane)
    {
        init();
    }

    GLuint getDepthMap() const { return depthMap; }
    GLuint getFBO() const { return fbo; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }

    ~SpotShadowMap()
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &depthMap);
    }

    SpotShadowMap(const SpotShadowMap&) = delete;
    SpotShadowMap& operator=(const SpotShadowMap&) = delete;

private:
    int width, height;
    GLuint fbo = 0;
    GLuint depthMap = 0;
    float nearPlane, farPlane;

    void init()
    {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &depthMap);

        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 
            0, 
            GL_DEPTH_COMPONENT, 
            width, 
            height, 
            0, 
            GL_DEPTH_COMPONENT, 
            GL_FLOAT, 
            // 只申请显存，不上传数据
            nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            depthMap,
            0);
        // 不需要颜色缓冲
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: SpotShadowMap framebuffer is not complete!" << std::endl;
        }

        // 恢复默认帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};