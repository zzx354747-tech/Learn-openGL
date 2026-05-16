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

struct SceneRenderConfig
{
    bool enableFloor = false;
    bool enableSkybox = false;
    bool enableFlashlight = false;
    bool enablePointLight = false;
    bool enableDirectionalLight = false;
};

struct SceneRenderResources
{
    Shader* basicCubeShader = nullptr;
    Shader* basicPlaneShader = nullptr;
    Shader* lightingCubeShader = nullptr;
    Shader* lightingPlaneShader = nullptr;
    Shader* lightCubeShader = nullptr;
    Shader* reflectShader = nullptr;

    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    LightMesh* lightMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;

    CubeMap* skybox = nullptr;
    GLTexture* floorTexture = nullptr;
};

struct SceneRenderState
{
    glm::vec3 cubePositions[3] = 
    {
        glm::vec3(-1.0f, 0.0f, -1.0f), 
        glm::vec3( 1.5f, 0.0f, -2.5f), 
        glm::vec3( 3.8f, 0.0f, -0.8f)  
    };

    glm::vec3 lightPositions = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f);
};

enum class RenderMode
{
    Basic,
    Lighting,
    Reflection
};

class SceneRender
{
public:
    
    SceneRender(
        SceneRenderResources resources,
        SceneRenderConfig config,
        SceneRenderState state,
        Camera& camera
    )        : resources(resources), config(config), state(state), camera(camera)
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
        framebuffer.bind();
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawCubeScene(bfwidth, bfheight, cubeTexture);
        drawPlaneScene(bfwidth, bfheight);
        drawLightcube(bfwidth, bfheight);
        drawSkyboxScene(bfwidth, bfheight);

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


private:
    Camera& camera;
    SceneRenderConfig config;
    SceneRenderResources resources;
    SceneRenderState state;

    RenderMode renderMode = RenderMode::Basic;

    void setupCamera(Shader &shader, int bfwidth, int bfheight)
    {
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        glm::mat4 projection = glm::perspective
        (
            glm::radians(45.0f), // 视野角（FOV）
            static_cast<float>(bfwidth) / static_cast<float>(bfheight), // 宽高比
            0.1f,  // 近裁剪面
            100.0f // 远裁剪面
        );
        shader.setMat4("projection", projection);
    }

    void setupPointLight(Shader& shader)
    {
        shader.setVec3("pointLight.position", state.lightPositions);
        shader.setVec3("pointLight.ambient", glm::vec3(0.2f));
        shader.setVec3("pointLight.diffuse", glm::vec3(0.8f));
        shader.setVec3("pointLight.specular", glm::vec3(1.0f));

        shader.setFloat("pointLight.constant", 1.0f);
        shader.setFloat("pointLight.linear", 0.09f);
        shader.setFloat("pointLight.quadratic", 0.032f);
    }

    void setupFlashLight(Shader& shader)
    {
        shader.setVec3("flashLight.position", camera.Getposition());
        shader.setVec3("flashLight.direction", camera.GetFront());

        shader.setVec3("flashLight.ambient", glm::vec3(0.1f));
        shader.setVec3("flashLight.diffuse", glm::vec3(0.8f));
        shader.setVec3("flashLight.specular", glm::vec3(1.0f));

        shader.setFloat("flashLight.constant", 1.0f);
        shader.setFloat("flashLight.linear", 0.09f);
        shader.setFloat("flashLight.quadratic", 0.032f);

        shader.setFloat("flashLight.cutOff",
            glm::cos(glm::radians(12.5f)));

        shader.setFloat("flashLight.outerCutOff",
            glm::cos(glm::radians(17.5f)));
    }

    void setupDirectionalLight(Shader& shader)
    {
        shader.setVec3("sun.direction",
                    glm::vec3(-0.2f, -1.0f, -0.3f));

        shader.setVec3("sun.ambient",
                    glm::vec3(0.2f, 0.2f, 0.2f));

        shader.setVec3("sun.diffuse",
                    glm::vec3(0.5f, 0.5f, 0.5f));

        shader.setVec3("sun.specular",
                    glm::vec3(1.0f, 1.0f, 1.0f));
    }

    void setupCubeLighting(Shader& shader)
    {
        if (config.enablePointLight)
            setupPointLight(shader);

        if (config.enableDirectionalLight)
            setupDirectionalLight(shader);

        if (config.enableFlashlight)
            setupFlashLight(shader);
    }

    void drawSkyboxScene(int bfwidth, int bfheight)
    {
        if (!config.enableSkybox || 
            !resources.reflectShader || 
            !resources.skybox ||
            !resources.skyboxMesh)
            return;

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        resources.reflectShader->use();
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        resources.reflectShader->setMat4("view", view);
        resources.reflectShader->setInt("skybox", 0);
        glm::mat4 projection = glm::perspective
        (
            glm::radians(45.0f), // 视野角（FOV）
            static_cast<float>(bfwidth) / static_cast<float>(bfheight), // 宽高比
            0.1f,  // 近裁剪面
            100.0f // 远裁剪面
        );
        resources.reflectShader->setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        resources.skybox->bind();
        // 绘制天空盒
         
        resources.skyboxMesh->draw();

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE); 
       
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

        setupCamera(*resources.lightCubeShader, bfwidth, bfheight);

        resources.lightMesh->draw();
    }

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture)
    {
        Shader* shader = nullptr;
        
        switch (renderMode)
        {
            case RenderMode::Basic:
            shader = resources.basicCubeShader;
            break;
            case RenderMode::Lighting:
            shader = resources.lightingCubeShader;
            break;
        
            case RenderMode::Reflection:
            shader = resources.reflectShader;
            break;
        }

        if (!shader || !resources.cubeMesh)
            return;
        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
        }

        setupCamera(*shader, bfwidth, bfheight);

        if (renderMode == RenderMode::Reflection)
        {
            if (!resources.skybox)
                return;

            shader->setInt("skybox", 0);
            glActiveTexture(GL_TEXTURE0);
            resources.skybox->bind();
        }
        else
        {
            shader->setInt("texture1", 0);
            glActiveTexture(GL_TEXTURE0);
            cubeTexture.bind();
        }

        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, state.cubePositions[i]);
            float angle = 20.0f * i; 
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shader->setMat4("model", model);
            resources.cubeMesh->draw();
        }
    }

    void drawPlaneScene(int bfwidth, 
        int bfheight)
    {
        Shader* shader = nullptr;
        switch (renderMode)
        {        
            case RenderMode::Basic:
            shader = resources.basicPlaneShader;
            break;
            case RenderMode::Lighting:
            shader = resources.lightingPlaneShader;
            break;
        
            case RenderMode::Reflection:
            shader = resources.basicPlaneShader;
            break;
        }
        if (!shader ||
            !resources.planeMesh||
            !resources.floorTexture||
            !config.enableFloor)
            return;

        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
        }

        glm::mat4 model = glm::mat4(1.0f);
        shader->setMat4("model", model);

        setupCamera(*shader, bfwidth, bfheight);

        shader->setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        resources.floorTexture->bind();
        resources.planeMesh->draw();
    }

};
