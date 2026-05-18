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

struct SetupLightdata
{
    glm::vec3 Pposition = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 Pambient = glm::vec3(0.2f);
    glm::vec3 Pdiffuse = glm::vec3(0.8f);
    glm::vec3 Pspecular = glm::vec3(1.0f);

    glm::vec3 Ddirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 Dambient = glm::vec3(0.05f);
    glm::vec3 Ddiffuse = glm::vec3(0.4f);
    glm::vec3 Dspecular = glm::vec3(0.5f);

    glm::vec3 Fambient = glm::vec3(0.0f);
    glm::vec3 Fdiffuse = glm::vec3(1.0f);
    glm::vec3 Fspecular = glm::vec3(1.0f);

    float Pconstant = 1.0f;
    float Plinear = 0.09f;
    float Pquadratic = 0.032f;

    float Fconstant = 1.0f; 
    float Flinear = 0.09f;
    float Fquadratic = 0.032f;

    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));

};

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

    void setLightData(SetupLightdata newData)
    {
        data = newData;
    }


private:
    Camera& camera;
    SceneRenderConfig config;
    SceneRenderResources resources;
    SceneRenderState state;
    SetupLightdata data;

    RenderMode renderMode = RenderMode::Basic;

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

    bool bindCubeTexture(Shader& shader, GLTexture& cubeTexture) const
    {
        if (renderMode == RenderMode::Reflection)
        {
            if (!resources.skybox)
                return false;
            glActiveTexture(GL_TEXTURE0);
            shader.setBool("isSkybox", false);
            shader.setInt("skybox", 0);
            shader.setVec3("cameraPos", camera.Getposition());
            resources.skybox->bind();
            return true;
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("texture1", 0);
            cubeTexture.bind();
            return true;
        }
    }

    void drawCube(Shader& shader)
    {
        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, state.cubePositions[i]);
            float angle = 20.0f * i; 
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shader.setMat4("model", model);
            resources.cubeMesh->draw();
        }
    }

    bool bindPlaneTexture(Shader& shader, GLTexture& floorTexture) const
    {
        if (!config.enableFloor || !resources.floorTexture)
            return false;
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("texture1", 0);
        floorTexture.bind();
        return true;
    }

    void drawPlane(Shader& shader)
    {
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        resources.planeMesh->draw();
    }

    bool bindSkyboxTexture(Shader& shader) const
    {
        if (!config.enableSkybox || !resources.skybox)
            return false;
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("skybox", 0);
        resources.skybox->bind();
        return true;
    }

    void drawSkybox(Shader& shader, int bfwidth, int bfheight)
    {
        shader.use();

        if (!bindSkyboxTexture(shader))
            return;

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        setupSkyboxCamera(shader, bfwidth, bfheight);

        shader.setBool("isSkybox", true);
        resources.skyboxMesh->draw();

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE); 
    }
            

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

    void setupSkyboxCamera(Shader &shader, int bfwidth, int bfheight)
    {
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
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
        shader.setVec3("pointLight.ambient", data.Pambient);
        shader.setVec3("pointLight.diffuse", data.Pdiffuse);
        shader.setVec3("pointLight.specular", data.Pspecular);

        shader.setFloat("pointLight.constant", data.Pconstant);
        shader.setFloat("pointLight.linear", data.Plinear);
        shader.setFloat("pointLight.quadratic", data.Pquadratic);
    }

    void setupFlashLight(Shader& shader)
    {
        shader.setVec3("flashLight.position", camera.Getposition());
        shader.setVec3("flashLight.direction", camera.GetFront());

        shader.setVec3("flashLight.ambient", data.Fambient);
        shader.setVec3("flashLight.diffuse", data.Fdiffuse);
        shader.setVec3("flashLight.specular", data.Fspecular);

        shader.setFloat("flashLight.constant", data.Fconstant);
        shader.setFloat("flashLight.linear", data.Flinear);
        shader.setFloat("flashLight.quadratic", data.Fquadratic);

        shader.setFloat("flashLight.cutOff", data.cutOff);
        shader.setFloat("flashLight.outerCutOff", data.outerCutOff);
    }

    void setupDirectionalLight(Shader& shader)
    {
        shader.setVec3("sun.direction", data.Ddirection);
        shader.setVec3("sun.ambient", data.Dambient);
        shader.setVec3("sun.diffuse", data.Ddiffuse);
        shader.setVec3("sun.specular", data.Dspecular);
    }

    void setupCubeLighting(Shader& shader)
    {
        shader.setVec3("viewPos", camera.Getposition());
        shader.setBool("enablePointLight", config.enablePointLight);
        shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
        shader.setBool("enableFlashlight", config.enableFlashlight);

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

            drawSkybox(*resources.reflectShader, bfwidth, bfheight);
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
        Shader* shader = getCubeShader();
        
        if (!shader || !resources.cubeMesh)
            return;
        shader->use();

        if (renderMode == RenderMode::Lighting)
        {
            setupCubeLighting(*shader);
        }

        setupCamera(*shader, bfwidth, bfheight);

        if (!bindCubeTexture(*shader, cubeTexture))
            return;

        drawCube(*shader);
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
        }

        setupCamera(*shader, bfwidth, bfheight);

        if (!bindPlaneTexture(*shader, *resources.floorTexture))
            return;

        drawPlane(*shader);
    }

};
