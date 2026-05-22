#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "rendering/assets/Texture.h"
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/assets/CubeMap.h"
#include "rendering/postprocess/Framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/postprocess/DirectionalShadowMap.h"
#include "rendering/Model/Mesh.h"
#include "rendering/Model/Model.h"
#include "rendering/passes/SkyboxPass.h"
#include "rendering/passes/LightVisualPass.h"





class SceneRender
{
public: 
    SceneRender(
        SceneRenderResources1 resources1,
        ShadowResources1 shadowResources1,
        SceneRenderConfig config,
        SceneRenderState state,
        Camera& camera,
        SceneDrawer& drawer
    )        : resources1(resources1),
        shadowResources1(shadowResources1),
        config(config), 
        state(state), 
        camera(camera), 
        drawer(drawer)
    {
    }

    void render(
        int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture, 
        Shader& screenShader, 
        Screenquad& screenQuad, 
        Framebuffer& framebuffer
    )
    {
        shadowResources1.shadowMap->renderShadowMap();

        if (renderMode == RenderMode::ShadowDebug && resources1.shadowDebugShader)
        {
            glViewport(0, 0, bfwidth, bfheight);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 显示深度图的shader绑定生成深度图的texture
            resources1.shadowDebugShader->use();
            resources1.shadowDebugShader->setInt("depthMap", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowResources1.shadowMap->getDepthMapTexture());
            screenQuad.draw();
            return;
        }

        framebuffer.bind();
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawCubeScene(bfwidth, bfheight, cubeTexture);
        drawPlaneScene(bfwidth, bfheight);
        lightVisualPass::renderLightVisualPass(camera, resources1, state, config, bfwidth, bfheight);

        if (resources1.model && resources1.modelShader)
        {
            draw3DModel(*resources1.model, *resources1.modelShader, bfwidth, bfheight);
        }

        SkyboxPass::renderSkyboxPass(camera, resources1, config, bfwidth, bfheight);

        framebuffer.unbind();
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

    void setRenderMode(RenderMode mode)
    {
        renderMode = mode;
    }

    void setconfig(SceneRenderConfig newConfig)
    {
        config = newConfig;
    }

    void setLightData(const LightSettings& newData)
    {
        lightSettings = newData;
    }


private:
    Camera& camera;
    SceneDrawer& drawer;
    SceneRenderConfig config;
    SceneRenderResources1 resources1;
    ShadowResources1 shadowResources1;
    SceneRenderState state;
    LightSettings lightSettings;
    RenderMode renderMode = RenderMode::Basic;

    void draw3DModel(Model& model, Shader& shader, int bfwidth, int bfheight)
    {
        shader.use();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        if (model.hasValidBounds())
        {
            glm::vec3 boundsCenter = model.getBoundsCenter();
            glm::vec3 boundsSize = model.getBoundsSize();
            float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
            float scale = maxExtent > 0.0f ? 10.0f / maxExtent : 1.0f;
            float floorY = -0.5f;
            glm::vec3 targetCenter(
                0.0f,
                floorY + boundsSize.y * scale * 0.5f,
                -2.0f
            );

            modelMatrix = glm::translate(modelMatrix, targetCenter);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
            modelMatrix = glm::translate(modelMatrix, -boundsCenter);
        }
        shader.setMat4("model", modelMatrix);

        CameraUniformSetter::apply(shader, camera, bfwidth, bfheight);

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(shader);
            setupCommonShadowUniforms(shader);
        }

        // 绘制模型
        model.draw(shader);
    }

    void setupCubeShadowUniforms(Shader& shader)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowResources1.shadowMap->getDepthMapTexture());

        ShadowMapUniformSetter::apply(shader);

        shader.setInt("shadowMap", 1);
    }

    void setupCommonShadowUniforms(Shader& shader)
{
    ShadowMapUniformSetter::apply(shader); 

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadowResources1.shadowMap->getDepthMapTexture());
    shader.setInt("shadowMap", 10);
}

    Shader* getCubeShader() const
    {
        switch (renderMode)
        {
            case RenderMode::Basic:
            return  resources1.basicCubeShader;

            case RenderMode::Lighting:
            return resources1.lightingCubeShader;
        
            case RenderMode::Reflection:
            return resources1.reflectShader;
            
            default:
            return nullptr;
        }
    }

    Shader* getPlaneShader() const
    {
        switch (renderMode)
        {
            case RenderMode::Basic:
                return resources1.basicPlaneShader;

            case RenderMode::Lighting:
                return resources1.lightingPlaneShader;

            case RenderMode::Reflection:
                return resources1.basicPlaneShader;

            default:
                return nullptr;
        }
    }

    void bindCubeTexture(Shader& shader, GLTexture& cubeTexture)
    {
        if (renderMode == RenderMode::Reflection)
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setBool("isSkybox", false);
            shader.setInt("skybox", 0);
            shader.setVec3("cameraPos", camera.Getposition());
            resources1.skybox->bind();
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("texture1", 0);
            cubeTexture.bind();
        }
    }

    void bindPlaneTexture(Shader& shader, GLTexture& floorTexture) const
    {
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("texture1", 0);
        floorTexture.bind();
    }

    void setupCubeLighting(Shader& shader)
    {
        shader.setVec3("viewPos", camera.Getposition());
        shader.setBool("enablePointLight", config.enablePointLight);
        shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
        shader.setBool("enableFlashlight", config.enableFlashlight);

        LightUniformSetter::apply(shader, lightSettings, config, state, camera);
    }

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture)
    {
        Shader* shader = getCubeShader();
        
        if (!shader || !resources1.cubeMesh)
            return;
        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
            setupCubeShadowUniforms(*resources1.lightingCubeShader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        
        if (renderMode == RenderMode::Reflection && !resources1.skybox)
            return;

        bindCubeTexture(*shader, cubeTexture);

        drawer.drawCubes(*shader);
    }

    void drawPlaneScene(int bfwidth, 
        int bfheight)
    {
        Shader* shader = getPlaneShader();
    
        if (!shader ||
            !resources1.planeMesh||
            !resources1.floorTexture||
            !config.enableFloor)
            return;

        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
            setupCubeShadowUniforms(*resources1.lightingPlaneShader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        bindPlaneTexture(*shader, *resources1.floorTexture);

        drawer.drawPlane(*shader);
    }

};
