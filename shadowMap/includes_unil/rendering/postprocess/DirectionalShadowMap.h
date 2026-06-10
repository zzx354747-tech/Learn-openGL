#pragma once
#include <glad/gl.h>
#include <iostream>
#include "core/shader.h"
#include "rendering/uniforms/ShadowMapUniformSetter.h"
#include "rendering/core/SceneDrawer.h"

// 这个类负责第一阶段的渲染
class DirectionalShadowMap
{
public:
    DirectionalShadowMap(Shader& shadowMapShader, 
        SceneDrawer& drawer,
        int width, 
        int height)

        : ShadowMapShader(shadowMapShader), 
        drawer(drawer),
        width(width), 
        height(height)

    {
        initShadowMap();
    }

    // 给第一阶段渲染准备的函数，渲染场景的深度信息到深度纹理中
    void renderShadowMap()
    {
        ShadowMapShader.use();
        ShadowMapUniformSetter::apply(ShadowMapShader);

        // 开启深度测试，防止某个pass关闭了深度测试导致这里出现问题
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        drawer.drawScene(ShadowMapShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 获取深度纹理的ID，以便在其他地方绑定使用
    unsigned int getDepthMapTexture() const
    {
        return depthMap;
    }

    DirectionalShadowMap(const DirectionalShadowMap&) = delete;
    DirectionalShadowMap& operator=(const DirectionalShadowMap&) = delete;

    ~DirectionalShadowMap()
    {
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthMap);
    }

private:
    Shader& ShadowMapShader;
    SceneDrawer& drawer;
    int width;
    int height;
    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;

    void initShadowMap()
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

};
