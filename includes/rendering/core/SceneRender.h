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
#include "rendering/assets/CubeMap.h"
#include "rendering/postprocess/framebuffer.h"
#include "rendering/postprocess/Screenquad.h"

class SceneRender
{
public:
    SceneRender(Shader& shader,
            Camera& camera,
            CubeMesh& cubeMesh  
            )   
        : shader(shader), camera(camera), cubeMesh(cubeMesh)
    {} 

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

    void setSkybox(Shader& skyboxShader, CubeMap& skybox)
    {
        this->skyboxShader = &skyboxShader;
        this->skybox = &skybox;
    }

    void setPlaneShader(Shader& planeShader, PlaneMesh& planeMesh, GLTexture& floorTexture)
    {
        this->planeShader = &planeShader;
        this->planeMesh = &planeMesh;
        this->floorTexture = &floorTexture;
        this->enableFloor = true;
    }

private:
    Shader& shader;
    Camera& camera;
    CubeMesh& cubeMesh;
    PlaneMesh* planeMesh = nullptr;
    GLTexture* floorTexture = nullptr;
    bool enableFloor = false;
    Shader* skyboxShader = nullptr;
    Shader* planeShader = nullptr;
    CubeMap* skybox = nullptr;

    glm::vec3 cubePositions[3] = 
    {
    glm::vec3(-1.0f, 0.0f, -1.0f), 
    glm::vec3( 1.5f, 0.0f, -2.5f), 
    glm::vec3( 3.8f, 0.0f, -0.8f)  
    };

    void drawSkyboxScene(int bfwidth, int bfheight)
    {
        if (!skyboxShader || !skybox)
            return;

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        skyboxShader->use();
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader->setMat4("view", view);
        skyboxShader->setInt("skybox", 0);
        glm::mat4 projection = glm::perspective
        (
            glm::radians(45.0f), // 视野角（FOV）
            static_cast<float>(bfwidth) / static_cast<float>(bfheight), // 宽高比
            0.1f,  // 近裁剪面
            100.0f // 远裁剪面
        );
        skyboxShader->setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        skybox->bind();
        // 绘制天空盒
         
        cubeMesh.draw();

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE); 
       
    }

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture)
    {
        if (!skybox)
            return;
        shader.use();
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
        shader.setVec3("cameraPos", camera.Getposition());

        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            shader.setMat4("model", model);
            shader.setInt("texture1", 0);
            glActiveTexture(GL_TEXTURE0);
            skybox->bind();
            cubeMesh.draw();
        }
    }

    void drawPlaneScene(int bfwidth, 
        int bfheight)
    {
        if (!enableFloor || !planeShader || !planeMesh || !floorTexture)
            return;
        planeShader->use();
        glm::mat4 model = glm::mat4(1.0f);
        planeShader->setMat4("model", model);
        glm::mat4 view = camera.GetViewMatrix();
        planeShader->setMat4("view", view);
        glm::mat4 projection = glm::perspective
        (
            glm::radians(45.0f), // 视野角（FOV）
            static_cast<float>(bfwidth) / static_cast<float>(bfheight), // 宽高比
            0.1f,  // 近裁剪面
            100.0f // 远裁剪面
        );
        planeShader->setMat4("projection", projection);

        planeShader->setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        floorTexture->bind();
        planeMesh->draw();
    }

};
