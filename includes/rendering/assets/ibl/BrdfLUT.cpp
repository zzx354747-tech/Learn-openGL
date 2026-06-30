#include "rendering/assets/ibl/BrdfLUT.h"
#include <iostream>

static float s_quadVertices[] = {
    // positions        // uvs
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
};

BrdfLUT::BrdfLUT(Shader& shader)
{
    Generate(shader);
}

BrdfLUT::~BrdfLUT()
{
    if (m_lutID)      glDeleteTextures(1, &m_lutID);
    if (m_captureFBO) glDeleteFramebuffers(1, &m_captureFBO);
    if (m_quadVAO)    glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO)    glDeleteBuffers(1, &m_quadVBO);
}

void BrdfLUT::Generate(Shader& shader)
{
    // 1. 创建 RG16F 纹理
    glGenTextures(1, &m_lutID);
    glBindTexture(GL_TEXTURE_2D, m_lutID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F,
                 SIZE, SIZE, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 2. FBO
    glGenFramebuffers(1, &m_captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_lutID, 0);

    // 3. 渲染一次全屏四边形
    glViewport(0, 0, SIZE, SIZE);
    shader.use();
    glClear(GL_COLOR_BUFFER_BIT);
    RenderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BrdfLUT::RenderQuad()
{
    if (m_quadVAO == 0) {
        glGenVertexArrays(1, &m_quadVAO);
        glGenBuffers(1, &m_quadVBO);

        glBindVertexArray(m_quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_quadVertices),
                     s_quadVertices, GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), (void*)0);
        // uv
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int BrdfLUT::GetID() const
{ return m_lutID; }

bool BrdfLUT::isReady() const
{ return m_lutID != 0; }
