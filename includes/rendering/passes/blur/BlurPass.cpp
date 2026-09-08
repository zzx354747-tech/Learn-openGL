#include "BlurPass.h"

BloomPass::BloomPass(int width, int height,
    Shader& downSampleShader, Shader& upSampleShader, Screenquad& quad)
    :   downSamplePassL1(width / 2, height / 2, downSampleShader, quad),
        downSamplePassL2(width / 4, height / 4, downSampleShader, quad),
        downSamplePassL3(width / 8, height / 8, downSampleShader, quad),
        downSamplePassL4(width / 16, height / 16, downSampleShader, quad),
        downSamplePassL5(width / 32, height / 32, downSampleShader, quad),
        upsamplePass(upSampleShader, quad)
{

}

void BloomPass::render(GLuint inputTexture)
{
    // Downsample
    downSamplePassL1.render(inputTexture);
    downSamplePassL2.render(downSamplePassL1.GetDownSampleTextureID());
    downSamplePassL3.render(downSamplePassL2.GetDownSampleTextureID());
    downSamplePassL4.render(downSamplePassL3.GetDownSampleTextureID());
    downSamplePassL5.render(downSamplePassL4.GetDownSampleTextureID());

    // Upsample and blend
    // L5 → L4
    upsamplePass.render(
        downSamplePassL4.GetDownSampleWidth(),
        downSamplePassL4.GetDownSampleHeight(),
        downSamplePassL5.GetDownSampleTextureID(),
        downSamplePassL4.GetDownSampleFBO(),
        1.0f);

    // L4 → L3
    upsamplePass.render(
        downSamplePassL3.GetDownSampleWidth(),
        downSamplePassL3.GetDownSampleHeight(),
        downSamplePassL4.GetDownSampleTextureID(),
        downSamplePassL3.GetDownSampleFBO(),
        1.0f);

    // L3 → L2
    upsamplePass.render(
        downSamplePassL2.GetDownSampleWidth(),
        downSamplePassL2.GetDownSampleHeight(),
        downSamplePassL3.GetDownSampleTextureID(),
        downSamplePassL2.GetDownSampleFBO(),
        1.0f);

    // L2 → L1
    upsamplePass.render(
        downSamplePassL1.GetDownSampleWidth(),
        downSamplePassL1.GetDownSampleHeight(),
        downSamplePassL2.GetDownSampleTextureID(),
        downSamplePassL1.GetDownSampleFBO(),
        1.0f);
}

void BloomPass::resize(int width, int height)
{
    downSamplePassL1.resize(width / 2, height / 2);
    downSamplePassL2.resize(width / 4, height / 4);
    downSamplePassL3.resize(width / 8, height / 8);
    downSamplePassL4.resize(width / 16, height / 16);
    downSamplePassL5.resize(width / 32, height / 32);
}
