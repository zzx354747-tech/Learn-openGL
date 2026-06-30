#include "rendering/resources/ssao/SSAO.h"
#include <random>
#include <iostream>

SSAO::SSAO(int width, int height)
{
    init(width, height);
}

void SSAO::init(int width, int height)
{
    // 创建一个随机数生成器
    std::default_random_engine generator;
    // 创建一个随机数生成器的分布规则，范围在0.0到1.0之间
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);

    // 生成一个矩形随机采样核，包含64个采样点
    for (int i = 0; i < 64; ++i)
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
        float scale = static_cast<float>(i) / 64.0f;
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
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
            std::cerr << "SSAO Blur Framebuffer not complete!" << std::endl;
        }

        // 创建SSAO_BlurFBO
        glGenFramebuffers(1, &SSAO_BlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, SSAO_BlurFBO);

        // 创建SSAO_BlurColorBuffer纹理
        glGenTextures(1, &SSAO_BlurColorBuffer);
        glBindTexture(GL_TEXTURE_2D, SSAO_BlurColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // 将SSAO_BlurColorBuffer纹理附加到SSAO_BlurFBO的颜色附件上
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            SSAO_BlurColorBuffer,
            0);

        // 检查SSAO_BlurFBO是否完整
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "SSAO Framebuffer not complete!" << std::endl;
        }

        // 解绑帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::resize(int width, int height)
{
    // 重新创建SSAO_ColorBuffer纹理
    glBindTexture(GL_TEXTURE_2D, SSAO_ColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);

    // 重新创建SSAO_BlurColorBuffer纹理
    glBindTexture(GL_TEXTURE_2D, SSAO_BlurColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

SSAO::~SSAO()
{
    glDeleteFramebuffers(1, &SSAO_FBO);
    glDeleteFramebuffers(1, &SSAO_BlurFBO);
    glDeleteTextures(1, &SSAO_ColorBuffer);
    glDeleteTextures(1, &SSAO_BlurColorBuffer);
    glDeleteTextures(1, &noiseTexture);
}

unsigned int SSAO::getNoiseTexture() const
{ return noiseTexture; }

unsigned int SSAO::getSSAOFBO() const
{ return SSAO_FBO; }

unsigned int SSAO::getSSAOBlurFBO() const
{ return SSAO_BlurFBO; }

unsigned int SSAO::getSSAOColorBuffer() const
{ return SSAO_ColorBuffer; }

unsigned int SSAO::getSSAOBlurColorBuffer() const
{ return SSAO_BlurColorBuffer; }

const std::vector<glm::vec3>& SSAO::getSSAOKernel() const
{ return ssaoKernel; }
