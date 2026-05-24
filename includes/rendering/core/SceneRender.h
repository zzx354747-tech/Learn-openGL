#pragma once
#include <glad/gl.h>
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/postprocess/Framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/passes/SkyboxPass.h"
#include "rendering/passes/LightVisualPass.h"
#include "rendering/passes/SceneObjectPass.h"
#include "rendering/passes/ShadowPass/PointShadowPass.h"

class SceneRender
{
public: 
    SceneRender(
        Camera& camera,
        SceneObjectPass& objectPass,
        ShadowResources& shadowResources,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        LightSettings& lightSettings,
        PointShadowPass& pointShadowPass
    )        : camera(camera),
        objectPass(objectPass),
        shadowResources(shadowResources),
        resources(resources),
        config(config), 
        state(state),
        lightSettings(lightSettings),
        pointShadowPass(pointShadowPass)
    {
    }

    void render(
        int bfwidth, 
        int bfheight, 
        Shader& screenShader, 
        Screenquad& screenQuad, 
        Framebuffer& framebuffer
    )
    {
        // 阴影图生成阶段
        shadowResources.shadowMap->renderShadowMap();
        pointShadowPass.render(state.lightPositions);

        if (config.renderMode == RenderMode::ShadowDebug && resources.shadowDebugShader)
        {
            renderShadowDebugPass(bfwidth, bfheight, screenQuad);
            return;
        }

        // 离屏渲染阶段
        renderFrameBufferPass(bfwidth, bfheight, framebuffer);

        // 屏幕渲染阶段
        renderScreenPass(bfwidth, bfheight, screenShader, screenQuad, framebuffer);
    }

private:
    Camera& camera;
    SceneRenderConfig& config;
    SceneRenderState& state;
    ShadowResources & shadowResources;
    SceneRenderResources& resources;
    LightSettings& lightSettings;
    SceneObjectPass& objectPass;
    PointShadowPass& pointShadowPass;

    void renderShadowDebugPass(int bfwidth, int bfheight, Screenquad& screenquad)
    {
        if (!resources.shadowDebugShader || !shadowResources.shadowMap)
            return;

        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 显示深度图的shader绑定生成深度图的texture
        resources.shadowDebugShader->use();
        resources.shadowDebugShader->setInt("depthMap", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowResources.shadowMap->getDepthMapTexture());
        screenquad.draw();
    }

    void renderFrameBufferPass(int bfwidth,
        int bfheight, 
        Framebuffer& framebuffer)
    {
       framebuffer.bind();
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        objectPass.renderCube(bfwidth, bfheight);
        objectPass.renderPlane(bfwidth, bfheight);
        lightVisualPass::renderLightVisualPass(camera, resources, state, config, bfwidth, bfheight);

        if (resources.model && resources.modelShader)
        {
            objectPass.renderModel(*resources.model, bfwidth, bfheight);
        }

        SkyboxPass::renderSkyboxPass(camera, resources, config, bfwidth, bfheight);

        framebuffer.unbind();
    }

    void renderScreenPass(int bfwidth,
        int bfheight,
        Shader& screenShader,
        Screenquad& screenQuad,
        Framebuffer& framebuffer)
    {
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        screenShader.setInt("screenTexture", 0);
        glDisable(GL_DEPTH_TEST);
        // 绑定帧缓冲区的纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
        screenQuad.draw();
    }
};
