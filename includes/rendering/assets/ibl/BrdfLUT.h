#pragma once
#include <glad/gl.h>
#include "core/Shader.h"

class BrdfLUT {
public:
    explicit BrdfLUT(Shader& shader);
    ~BrdfLUT();

    BrdfLUT(const BrdfLUT&) = delete;
    BrdfLUT& operator=(const BrdfLUT&) = delete;

    unsigned int GetID() const;
    bool isReady() const;

private:
    void Generate(Shader& shader);
    void RenderQuad();

    unsigned int m_lutID      = 0;
    unsigned int m_captureFBO = 0;
    unsigned int m_quadVAO    = 0;
    unsigned int m_quadVBO    = 0;

    static constexpr unsigned int SIZE = 512;
};
