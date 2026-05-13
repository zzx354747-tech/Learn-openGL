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
#include "rendering/postprocess/framebuffer.h"
#include "rendering/postprocess/Screenquad.h"

class SceneRender
{
public:
    SceneRender(Shader& shader,
            Camera& camera,
            CubeMesh& cubeMesh,     
            PlaneMesh& planeMesh)   
        : shader(shader), camera(camera), cubeMesh(cubeMesh), planeMesh(planeMesh)
    {} 

    void render(
        int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture, 
        GLTexture& floorTexture, 
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
        drawPlaneScene(bfwidth, bfheight, floorTexture);

        framebuffer.unbind();
        glViewport(0, 0, bfwidth, bfheight);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        screenShader.setInt("screenTexture", 0);
        glDisable(GL_DEPTH_TEST);
        // 绑定帧缓冲区的纹理
        glBindTexture(GL_TEXTURE_2D, framebuffer.getTextureID());
        screenQuad.draw();
    }

private:
    Shader& shader;
    Camera& camera;
    CubeMesh& cubeMesh;
    PlaneMesh& planeMesh;

    glm::vec3 cubePositions[3] = 
    {
    glm::vec3(-1.0f, 0.0f, -1.0f), 
    glm::vec3( 1.5f, 0.0f, -2.5f), 
    glm::vec3( 3.8f, 0.0f, -0.8f)  
    };

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        GLTexture& cubeTexture)
    {
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

        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            shader.setMat4("model", model);
            cubeTexture.bind();
            cubeMesh.draw();
        }
    }

    void drawPlaneScene(int bfwidth, 
        int bfheight,
        GLTexture& floorTexture)
    {
        shader.use();
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
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

        floorTexture.bind();
        planeMesh.draw();
    }

};