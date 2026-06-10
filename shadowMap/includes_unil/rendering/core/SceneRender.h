#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "rendering/assets/texture.h"
#include "core/shader.h"
#include "scene/camera.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/assets/CubeMap.h"
#include "rendering/postprocess/framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/uniforms/CameraUniformSetter.h"
#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/postprocess/DirectionalShadowMap.h"

struct SceneRenderResources
{
    Shader* basicCubeShader = nullptr;
    Shader* basicPlaneShader = nullptr;
    Shader* lightingCubeShader = nullptr;
    Shader* lightingPlaneShader = nullptr;
    Shader* lightCubeShader = nullptr;
    Shader* reflectShader = nullptr;
    Shader* shadowDebugShader = nullptr;
    Shader* shadowMapShader = nullptr;

    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    LightMesh* lightMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;

    CubeMap* skybox = nullptr;
    GLTexture* floorTexture = nullptr;
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
};

enum class RenderMode
{
    Basic,
    Lighting,
    Reflection,
    ShadowDebug,
};

class SceneRender
{
public:
    
    SceneRender(
        SceneRenderResources resources,
        ShadowResources shadowResources,
        SceneRenderConfig config,
        SceneRenderState state,
        Camera& camera,
        SceneDrawer& drawer
    )        : resources(resources), 
        shadowResources(shadowResources),   
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
        shadowResources.shadowMap->renderShadowMap();

        if (renderMode == RenderMode::ShadowDebug && resources.shadowDebugShader)
        {
            glViewport(0, 0, bfwidth, bfheight);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 显示深度图的shader绑定生成深度图的texture
            resources.shadowDebugShader->use();
            resources.shadowDebugShader->setInt("depthMap", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowResources.shadowMap->getDepthMapTexture());
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
        drawLightcube(bfwidth, bfheight);
        renderSkyboxPass(bfwidth, bfheight);

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
    SceneRenderResources resources;
    ShadowResources shadowResources;
    SceneRenderState state;
    LightSettings lightSettings;
    RenderMode renderMode = RenderMode::Basic;

    void setupShadowUniforms(Shader& shader)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowResources.shadowMap->getDepthMapTexture());

        shader.setInt("shadowMap", 1);

        ShadowMapUniformSetter::apply(shader);
    }

    Shader* getCubeShader() const
    {
        switch (renderMode)
        {
            case RenderMode::Basic:
            return resources.basicCubeShader;

            case RenderMode::Lighting:
            return resources.lightingCubeShader;
        
            case RenderMode::Reflection:
            return resources.reflectShader;
            
            default:
            return nullptr;
        }
    }

    Shader* getPlaneShader() const
    {
        switch (renderMode)
        {
            case RenderMode::Basic:
                return resources.basicPlaneShader;

            case RenderMode::Lighting:
                return resources.lightingPlaneShader;

            case RenderMode::Reflection:
                return resources.basicPlaneShader;

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
            resources.skybox->bind();
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

    void bindSkyboxTexture(Shader& shader) const
    {
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("skybox", 0);
        resources.skybox->bind();
    }

    void renderSkyboxPass(int bfwidth, int bfheight)
    {
        if (!resources.skyboxMesh||
            !resources.skybox||
            !config.enableSkybox||
            !resources.reflectShader)
            return;

        Shader& shader = *resources.reflectShader;
        shader.use();

        bindSkyboxTexture(shader);

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        SkyboxCameraUniformSetter::apply(shader, camera, bfwidth, bfheight);

        shader.setBool("isSkybox", true);
        resources.skyboxMesh->draw();

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE); 
    }

    void setupCubeLighting(Shader& shader)
    {
        shader.setVec3("viewPos", camera.Getposition());
        shader.setBool("enablePointLight", config.enablePointLight);
        shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
        shader.setBool("enableFlashlight", config.enableFlashlight);

        LightUniformSetter::apply(shader, lightSettings, config, state, camera);
    }

    void drawLightcube(int bfwidth, int bfheight)
    {
        if (!resources.lightCubeShader || 
            !resources.lightMesh||
            !config.enablePointLight)
            return;
        resources.lightCubeShader->use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, state.lightPositions);
        model = glm::scale(model, glm::vec3(0.2f)); // 将灯光立方体缩小
        resources.lightCubeShader->setMat4("model", model);

        CameraUniformSetter::apply(*resources.lightCubeShader, camera, bfwidth, bfheight);

        resources.lightMesh->draw();
    }

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture)
    {
        Shader* shader = getCubeShader();
        
        if (!shader || !resources.cubeMesh)
            return;
        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
            setupShadowUniforms(*resources.lightingCubeShader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        
        if (renderMode == RenderMode::Reflection && !resources.skybox)
            return;

        bindCubeTexture(*shader, cubeTexture);

        drawer.drawCubes(*shader);
    }

    void drawPlaneScene(int bfwidth, 
        int bfheight)
    {
        Shader* shader = getPlaneShader();
    
        if (!shader ||
            !resources.planeMesh||
            !resources.floorTexture||
            !config.enableFloor)
            return;

        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
            setupShadowUniforms(*resources.lightingPlaneShader);
        }

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        bindPlaneTexture(*shader, *resources.floorTexture);

        drawer.drawPlane(*shader);
    }

};
