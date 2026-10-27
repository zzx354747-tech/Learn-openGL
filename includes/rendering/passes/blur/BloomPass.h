#pragma once

#include <glad/gl.h>
#include <array>
#include "DownSamplePass.h"
#include "UpSamplePass.h"
#include "core/Shader.h"
#include "rendering/assets/mesh/Screenquad.h"

class BloomPass
{
public:
    BloomPass(int width, int height,
    Shader& downSampleShader, Shader& upSampleShader, Screenquad& quad);

    void render(GLuint inputTexture, const std::array<float, 5>& thresholds = {},
                float knee = 0.5f, int levels = 5, float filterRadius = 1.0f);
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
