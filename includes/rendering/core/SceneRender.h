#pragma once
#include <glad/gl.h>
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/postprocess/HDR_Framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/passes/SkyboxPass.h"
#include "rendering/passes/LightVisualPass.h"
#include "rendering/passes/SceneObjectPass.h"
#include "rendering/passes/ShadowPass/DirectionalShadowPass.h"
#include "rendering/passes/ShadowPass/PointShadowPass.h"
#include "rendering/passes/ShadowPass/SpotShadowPass.h"
#include "rendering/passes/GeometryPass.h"
#include "rendering/passes/LightingPass.h"
#include "rendering/postprocess/Gbuffer.h"

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
        DirectionalShadowPass& directionalShadowPass,
        PointShadowPass& pointShadowPass,
        SpotShadowPass& spotShadowPass,
        GeometryPass& geometryPass,
        LightingPass& lightingPass,
        GBuffer& gBuffer
    )        : camera(camera),
        objectPass(objectPass),
        shadowResources(shadowResources),
        resources(resources),
        config(config), 
        state(state),
        lightSettings(lightSettings),
        directionalShadowPass(directionalShadowPass),
        pointShadowPass(pointShadowPass),
        spotShadowPass(spotShadowPass),
        geometryPass(geometryPass),
        lightingPass(lightingPass),
        gBuffer(gBuffer)
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
        directionalShadowPass.render();
        pointShadowPass.render(state.lightPositions);
        spotShadowPass.render();

        if (config.renderMode == RenderMode::ShadowDebug && resources.shadowDebugShader)
        {
            renderShadowDebugPass(bfwidth, bfheight, screenQuad);
            return;
        }

        //Light模式下的延迟渲染阶段
        if (config.renderMode == RenderMode::Lighting)
        {
            renderGboPass(bfwidth, bfheight, framebuffer, screenQuad);
        }
        else
        {
        // 非Lighting模式下的离屏渲染阶段
        renderHDRFrameBufferPass(bfwidth, bfheight, framebuffer);
        }
        
        if (config.enableBloom && resources.pingpongFBO && resources.blurShader)
        {
            renderBlurPass(framebuffer, *resources.pingpongFBO, *resources.blurShader, screenQuad, config.numBlurPasses);
        }

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
    DirectionalShadowPass& directionalShadowPass;
    PointShadowPass& pointShadowPass;
    SpotShadowPass& spotShadowPass;
    GeometryPass& geometryPass;
    LightingPass& lightingPass;
    GBuffer& gBuffer;

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

    void renderGboPass(int bfwidth, int bfheight, Framebuffer& framebuffer, Screenquad& screenQuad)
    {
        geometryPass.render(bfwidth, bfheight);
        gBuffer.blitDepthTo(framebuffer, bfwidth, bfheight);
        lightingPass.render(framebuffer, screenQuad);

        //lighting模式下的skybox,light render
        framebuffer.bind();
        glViewport(0, 0, bfwidth, bfheight);
        glEnable(GL_DEPTH_TEST);
        lightVisualPass::renderLightVisualPass(camera, resources, state, config, bfwidth, bfheight);
        SkyboxPass::renderSkyboxPass(camera, resources, config, bfwidth, bfheight);
        framebuffer.unbind();
    }

    void renderHDRFrameBufferPass(int bfwidth,
        int bfheight, 
        Framebuffer& framebuffer)
    {
       framebuffer.bind();
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        objectPass.renderNoLightingCube(bfwidth, bfheight);
        objectPass.renderNoLightingPlane(bfwidth, bfheight);
        lightVisualPass::renderLightVisualPass(camera, resources, state, config, bfwidth, bfheight);

        if (resources.model)
        {
            objectPass.renderNoLightingModel(bfwidth, bfheight);
        }

        SkyboxPass::renderSkyboxPass(camera, resources, config, bfwidth, bfheight);

        framebuffer.unbind();
    }

     // 这个渲染流程只处理高亮场景的两个fbo
    void renderBlurPass(
        Framebuffer& framebuffer,
        PingPongFramebuffer& pingpong,
        Shader& blurShader,
        Screenquad& screenQuad,
        int amount)  // 模糊次数，必须是偶数
    {
        bool horizontal = true;
        bool firstIteration = true;

        glDisable(GL_DEPTH_TEST);
        blurShader.use();
        blurShader.setInt("image", 0);

        for (int i = 0; i < amount; ++i)
        {
            // 决定绑定到哪个fbo
            pingpong.bind(horizontal ? 0 : 1);

            // horizontal为true时，shader会在水平方向模糊，false时在垂直方向模糊
            // 动态变化，所以写成uniform
            blurShader.setBool("horizontal", horizontal);

            glActiveTexture(GL_TEXTURE0);
            // 决定纹理从哪个fbo读
            glBindTexture(GL_TEXTURE_2D,
                firstIteration
                    ? framebuffer.getTextureID(1)  // 第一次从高亮纹理读
                    : pingpong.getTextureID(horizontal ? 1 : 0)  // 之后从上一次结果读
            );

            // 多次draw，但是blur着色器不涉及光照计算，性能开销可以接受
            screenQuad.draw();

            // 切换模糊方向
            horizontal = !horizontal;
            // 标记第一次迭代已经完成，之后的迭代都从pingpong的结果读
            firstIteration = false;
        }

        pingpong.unbind();
    }

    // 叠加三个
    void renderScreenPass(int bfwidth, int bfheight,
    Shader& screenShader,
    Screenquad& screenQuad,
    Framebuffer& framebuffer)
    {
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        screenShader.use();
        screenShader.setInt("screenTexture", 0);
        screenShader.setInt("bloomBlur", 1);
        screenShader.setFloat("exposure", config.exposure);
        screenShader.setBool("enableHdr", config.enableHDR);
        screenShader.setBool("enableGamma", config.enableGammaCorrection);
        screenShader.setBool("enableBloom", config.enableBloom);
        screenShader.setFloat("bloomStrength", config.bloomStrength);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID(0));

        if (config.enableBloom && resources.pingpongFBO)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, resources.pingpongFBO->getTextureID(1));
        }

        screenQuad.draw();
    }

};
