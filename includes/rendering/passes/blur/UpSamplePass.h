#pragma once

#include <glad/gl.h>
#include "rendering/assets/mesh/Screenquad.h"
#include "core/Shader.h"

class UpsamplePass
{
public:
    UpsamplePass(Shader& shader, Screenquad& quad);

    void render(int width, int height,
    GLuint sourceTexture, GLuint targetFBO, float filterRadius);

private:
    Shader& upsampleShader;
    Screenquad& screenQuad;
};
