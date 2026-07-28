#pragma once

#include <glad/gl.h>

#include "core/Shader.h"
#include "rendering/resources/framebuffer/PingPong_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"

class BlurPass
{
public:
    void render(
        GLuint inputTexture,
        PingPongFramebuffer& pingpong,
        Shader& blurShader,
        Screenquad& screenQuad,
        int amount
    );
};
