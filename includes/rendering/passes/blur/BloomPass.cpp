#include "BloomPass.h"
#include <algorithm>

BloomPass::BloomPass(int width, int height,
    Shader& downSampleShader, Shader& upSampleShader, Screenquad& quad)
    :   downSamplePassL1(std::max(1, width / 2), std::max(1, height / 2), downSampleShader, quad),
        downSamplePassL2(std::max(1, width / 4), std::max(1, height / 4), downSampleShader, quad),
        downSamplePassL3(std::max(1, width / 8), std::max(1, height / 8), downSampleShader, quad),
        downSamplePassL4(std::max(1, width / 16), std::max(1, height / 16), downSampleShader, quad),
        downSamplePassL5(std::max(1, width / 32), std::max(1, height / 32), downSampleShader, quad),
        upsamplePass(upSampleShader, quad)
{

}

void BloomPass::render(GLuint inputTexture, const std::array<float, 5>& thresholds,
                       float knee, int levels, float filterRadius)
{
    DownSamplePass* chain[] = {&downSamplePassL1, &downSamplePassL2, &downSamplePassL3,
                               &downSamplePassL4, &downSamplePassL5};
    levels = std::clamp(levels, 1, 5);
    for (int level = 0; level < levels; ++level)
    {
        chain[level]->render(inputTexture, thresholds[level], knee, filterRadius);
        inputTexture = chain[level]->GetDownSampleTextureID();
    }
    // Only active levels are read, so shrinking the chain cannot reuse stale wide halos.
    for (int level = levels - 1; level > 0; --level)
        upsamplePass.render(chain[level - 1]->GetDownSampleWidth(),
                            chain[level - 1]->GetDownSampleHeight(),
                            chain[level]->GetDownSampleTextureID(),
                            chain[level - 1]->GetDownSampleFBO(), filterRadius);
}

void BloomPass::resize(int width, int height)
{
    downSamplePassL1.resize(std::max(1, width / 2), std::max(1, height / 2));
    downSamplePassL2.resize(std::max(1, width / 4), std::max(1, height / 4));
    downSamplePassL3.resize(std::max(1, width / 8), std::max(1, height / 8));
    downSamplePassL4.resize(std::max(1, width / 16), std::max(1, height / 16));
    downSamplePassL5.resize(std::max(1, width / 32), std::max(1, height / 32));
}
