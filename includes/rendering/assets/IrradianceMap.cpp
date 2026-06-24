#include "rendering/assets/IrradianceMap.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static float s_cubeVertices[] = {
    -1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
    -1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1,
    -1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1, 1,
     1, 1, 1,  1, 1,-1,  1,-1,-1,  1,-1, 1,
    -1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1, -1, 1, 1
};
static unsigned int s_cubeIndices[] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

IrradianceMap::IrradianceMap(const EnvCubemap& envCubemap, Shader& shader)
{
    if (!envCubemap.isReady()) {
        std::cerr << "[IrradianceMap] EnvCubemap not ready." << std::endl;
        return;
    }
    Convolve(envCubemap.getCubeMapID(), shader);
}

IrradianceMap::~IrradianceMap()
{
    if (m_irradianceID) glDeleteTextures(1, &m_irradianceID);
    if (m_captureFBO)   glDeleteFramebuffers(1, &m_captureFBO);
    if (m_cubeVAO)      glDeleteVertexArrays(1, &m_cubeVAO);
    if (m_cubeVBO)      glDeleteBuffers(1, &m_cubeVBO);
    if (m_cubeEBO)      glDeleteBuffers(1, &m_cubeEBO);
}

void IrradianceMap::Convolve(unsigned int envCubemapID, Shader& shader)
{
    // 1. 创建目标 irradiance cubemap
    glGenTextures(1, &m_irradianceID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceID);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     SIZE, SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 2. FBO
    glGenFramebuffers(1, &m_captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    // 3. 投影 + 六个面 view（和 EnvCubemap 完全相同）
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[] = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
    };

    // 4. 卷积六次
    shader.use();
    shader.setInt("environmentMap", 0);
    shader.setMat4("projection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemapID);

    glViewport(0, 0, SIZE, SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        shader.setMat4("view", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               m_irradianceID, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        RenderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IrradianceMap::RenderCube()
{
    if (m_cubeVAO == 0) {
        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        glBindVertexArray(m_cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_cubeVertices),
                     s_cubeVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_cubeIndices),
                     s_cubeIndices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
    glBindVertexArray(m_cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}