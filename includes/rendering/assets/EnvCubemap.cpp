#include "rendering/assets/EnvCubemap.h"
#include "stb_image.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>

static float s_cubeVertices[] = {
    -1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,  // -Z
    -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1,  // +Z
    -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1, 1,  // -X
     1, 1, 1,  1, 1,-1,  1,-1,-1,  1,-1, 1,  // +X
    -1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1,  // -Y
    -1, 1,-1,  1, 1,-1,  1, 1, 1, -1, 1, 1   // +Y
};

static unsigned int s_cubeIndices[] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

EnvCubemap::EnvCubemap(HDRTexture& hdrTexture, Shader& shader)
: hdrTexture(hdrTexture)
{
    convert(shader);
}

EnvCubemap::~EnvCubemap()
{
    if (cubeMapID)
    {
        glDeleteTextures(1, &cubeMapID);
    }
    if (cubeMapFBO)
    {
        glDeleteFramebuffers(1, &cubeMapFBO);
    }
    if (cubeVAO)
    {
        glDeleteVertexArrays(1, &cubeVAO);
    }
    if (cubeVBO)
    {
        glDeleteBuffers(1, &cubeVBO);
    }
    if (cubeEBO)
    {
        glDeleteBuffers(1, &cubeEBO);
    }
}

void EnvCubemap::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);
}

void EnvCubemap::unbind() const
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void EnvCubemap::convert(Shader &shader)
{
    if (!hdrTexture.isLoaded())
    {
        std::cerr << "HDR texture is not loaded. Cannot convert to EnvCubemap." << std::endl;
        return;
    }

    // 创建fbo
    glGenFramebuffers(1, &cubeMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);

    // 创建texture
    glGenTextures(1, &cubeMapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);

    // 设置每个面的纹理参数
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_RGB16F, SIZE, SIZE, 
            0, GL_RGB, GL_FLOAT, 
            nullptr);
    }

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[] = {
    glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
    };

    shader.use();
    shader.setInt("equirectangularMap", 0);
    shader.setMat4("projection", proj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture.getId());

    glViewport(0, 0, SIZE, SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, cubeMapFBO);
    // 没有rbo,关闭深度测试
    glDisable(GL_DEPTH_TEST);

    for (unsigned int i = 0; i < 6; ++i)
    {
        shader.setMat4("view", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMapID, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        RenderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EnvCubemap::RenderCube()
{
    if (cubeVAO == 0)
    {
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glGenBuffers(1, &cubeEBO);

        glBindVertexArray(cubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_cubeVertices), s_cubeVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_cubeIndices), s_cubeIndices, GL_STATIC_DRAW);

        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
