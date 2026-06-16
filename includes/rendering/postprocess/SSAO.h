#pragma once

#include <glad/gl.h>
#include <vector>
#include <glm/glm.hpp>

class SSAO
{
public:
    SSAO(int width, int height);

    void resize (int width, int height);

    unsigned int getNoiseTexture() const { return noiseTexture; }

    unsigned int getSSAOFBO() const { return SSAO_FBO; }
    unsigned int getSSAOBlurFBO() const { return SSAO_BlurFBO; }
    unsigned int getSSAOColorBuffer() const { return SSAO_ColorBuffer; }
    unsigned int getSSAOBlurColorBuffer() const { return SSAO_BlurColorBuffer; }

    const std::vector<glm::vec3>& getSSAOKernel() const { return ssaoKernel; }

    ~SSAO();

private:
    unsigned int SSAO_FBO, SSAO_BlurFBO;
    unsigned int SSAO_ColorBuffer, SSAO_BlurColorBuffer;
    unsigned int noiseTexture;
    std::vector<glm::vec3> ssaoKernel;

    void init(int width, int height);
};
