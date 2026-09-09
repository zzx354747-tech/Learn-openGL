#pragma once

#include <glad/gl.h>
#include "DownSamplePass.h"
#include "UpSamplePass.h"
#include "core/Shader.h"
#include "rendering/assets/mesh/Screenquad.h"

class BloomPass
{
public:
    BloomPass(int width, int height,
    Shader& downSampleShader, Shader& upSampleShader, Screenquad& quad);

    void render(GLuint inputTexture);
    void resize(int width, int height);
    GLuint GetBloomTextureID() const { return downSamplePassL1.GetDownSampleTextureID(); }

    private:
    DownSamplePass downSamplePassL1;
    DownSamplePass downSamplePassL2;
    DownSamplePass downSamplePassL3;
    DownSamplePass downSamplePassL4;
    DownSamplePass downSamplePassL5;
    UpsamplePass upsamplePass;
};
