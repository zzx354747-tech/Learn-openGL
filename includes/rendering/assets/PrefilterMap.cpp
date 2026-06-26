#include "rendering/assets/PrefilterMap.h"
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

PrefilterMap::PrefilterMap(const EnvCubemap& envCubemap, Shader& shader)
{
    if (!envCubemap.isReady()) {
        std::cerr << "[PrefilterMap] EnvCubemap not ready." << std::endl;
        return;
    }

    Prefilter(envCubemap.getCubeMapID(), shader);
}

PrefilterMap::~PrefilterMap()
{
    if (m_prefilterID) glDeleteTextures(1, &m_prefilterID);
    if (m_captureFBO)   glDeleteFramebuffers(1, &m_captureFBO);
    if (m_cubeVAO)      glDeleteVertexArrays(1, &m_cubeVAO);
    if (m_cubeVBO)      glDeleteBuffers(1, &m_cubeVBO);
    if (m_cubeEBO)      glDeleteBuffers(1, &m_cubeEBO);
}

void PrefilterMap::Prefilter(unsigned int envCubemapID, Shader& shader)
{
    // 1. 创建目标 prefilter cubemap
    glGenTextures(1, &m_prefilterID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterID);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     SIZE, SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 生成 mipmap
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // 2. FBO
    glGenFramebuffers(1, &m_captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    // 3. 投影 + 六个面 view（和 EnvCubemap 完全相同）
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    // 4. prefilter 六次
    shader.use();
    shader.setInt("environmentMap", 0);
    shader.setMat4("projection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemapID);
    //让EnvCubemap的生成mipmap
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    for (unsigned int mip = 0; mip < MAX_MIP_LEVELS; ++mip) {
        // 计算每个 mipmap 的尺寸
        unsigned int mipWidth  = static_cast<unsigned int>(SIZE * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(SIZE * std::pow(0.5, mip));
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = static_cast<float>(mip) / static_cast<float>(MAX_MIP_LEVELS - 1);
        shader.setFloat("roughness", roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            shader.setMat4("view", views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_prefilterID, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PrefilterMap::RenderCube()
{
    if (m_cubeVAO == 0) {
        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        glBindVertexArray(m_cubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_cubeVertices), s_cubeVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_cubeIndices), s_cubeIndices, GL_STATIC_DRAW);

        // 设置顶点属性指针
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    glBindVertexArray(m_cubeVAO);
    glDrawElements(GL_TRIANGLES, sizeof(s_cubeIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}