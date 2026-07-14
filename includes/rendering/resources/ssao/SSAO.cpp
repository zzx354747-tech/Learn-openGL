#include "rendering/resources/ssao/SSAO.h"
#include <random>
#include <iostream>

SSAO::SSAO(int width, int height)
    : bilateralBlurPingPong(width, height)
{
    init(width, height);
}

void SSAO::init(int width, int height)
{
    // 创建一个随机数生成器
    std::default_random_engine generator;
    // 创建一个随机数生成器的分布规则，范围在0.0到1.0之间
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    constexpr int kernelSize = 16;

    // 生成一个半球随机采样核。低采样数配合旋转噪声和双边滤波使用。
    for (int i = 0; i < kernelSize; ++i)
    {
        // 这一步+normalize  随机的意义在于：采样点在半球面上随机分布
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f, // x: [-1, 1]
            randomFloats(generator) * 2.0f - 1.0f, // y: [-1, 1]
            randomFloats(generator)                // z: [0, 1]
        );
        // 将采样点归一化，让它分布在本球上
        sample = glm::normalize(sample);

        // 将采样点分布随机打乱到半球内
        sample *= randomFloats(generator);

        // 线性插值过程，将采样点按二次函数分布在半球内，靠近中心的采样点更密集
        float scale = static_cast<float>(i) / static_cast<float>(kernelSize);
        scale = 0.1f + 0.9f * (scale * scale); // 线性插值，范围从0.1到1.0
        sample *= scale;

        ssaoKernel.push_back(sample);
        }

        // 4x4的随机旋转噪声纹理，包含16个随机旋转向量
        std::vector<glm::vec3> ssaoNoise;
        for (int i = 0; i < 16; i++)
        {
            glm::vec3 noise(
                randomFloats(generator) * 2.0f - 1.0f, // x: [-1, 1]
                randomFloats(generator) * 2.0f - 1.0f, // y: [-1, 1]
                0.0f                                   // z: 0
            );
            ssaoNoise.push_back(noise);
        }

        // 创建噪声纹理
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // 创建SSAO_FBO
        glGenFramebuffers(1, &SSAO_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, SSAO_FBO);

        // 创建SSAO_ColorBuffer纹理
        glGenTextures(1, &SSAO_ColorBuffer);
        glBindTexture(GL_TEXTURE_2D, SSAO_ColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // 将SSAO_ColorBuffer纹理附加到SSAO_FBO的颜色附件上
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            SSAO_ColorBuffer,
            0);

        // 检查SSAO_FBO是否完整
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "SSAO Framebuffer not complete!" << std::endl;
        }

        // 解绑帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::resize(int width, int height)
{
    // 重新创建原始 SSAO 纹理
    glBindTexture(GL_TEXTURE_2D, SSAO_ColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    bilateralBlurPingPong.resize(width, height);
}

SSAO::~SSAO()
{
    glDeleteFramebuffers(1, &SSAO_FBO);
    glDeleteTextures(1, &SSAO_ColorBuffer);
    glDeleteTextures(1, &noiseTexture);
}

unsigned int SSAO::getNoiseTexture() const
{ return noiseTexture; }

unsigned int SSAO::getSSAOFBO() const
{ return SSAO_FBO; }

unsigned int SSAO::getSSAOColorBuffer() const
{ return SSAO_ColorBuffer; }

unsigned int SSAO::getBilateralBlurTexture() const
{ return bilateralBlurPingPong.getTextureID(1); }

SSAOPingPongFramebuffer& SSAO::getBilateralBlurPingPong()
{ return bilateralBlurPingPong; }

const std::vector<glm::vec3>& SSAO::getSSAOKernel() const
{ return ssaoKernel; }
