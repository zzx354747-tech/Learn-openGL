#pragma once

#include <glad/gl.h>
#include <vector>
#include <glm/glm.hpp>

class SSAO
{
public:
    SSAO(int width, int height);

    SSAO(const SSAO&) = delete;
    SSAO& operator=(const SSAO&) = delete;

    void resize (int width, int height);

    unsigned int getNoiseTexture() const;

    unsigned int getSSAOFBO() const;
    unsigned int getSSAOBlurFBO() const;
    unsigned int getSSAOColorBuffer() const;
    unsigned int getSSAOBlurColorBuffer() const;

    const std::vector<glm::vec3>& getSSAOKernel() const;

    ~SSAO();

private:
    unsigned int SSAO_FBO, SSAO_BlurFBO;
    unsigned int SSAO_ColorBuffer, SSAO_BlurColorBuffer;
    unsigned int noiseTexture;
    std::vector<glm::vec3> ssaoKernel;

    void init(int width, int height);
};
