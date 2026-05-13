#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "rendering/texture.h"
#include "core/shader.h"
#include "scene/camera.h"

class RenderD1
{
public:
    RenderD1(Shader& shader,
            Camera& camera)
        : shader(shader), camera(camera)
    {

    }

private:
    Shader& shader;
    Camera& camera;
    unsigned int cubeVAO, planeVAO;

    void drawCubeScene(int bfwidth, 
        int bfheight, 
        glm::vec3 cubePositions[],
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
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);  
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
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

};