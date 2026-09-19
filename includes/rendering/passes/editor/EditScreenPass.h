#pragma once

#include "rendering/assets/mesh/Screenquad.h"
#include "core/Shader.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "scene/Camera.h"

class EditScreenPass
{
public:
    EditScreenPass(Screenquad& quad, Shader& shader, Camera& camera)
        : screenQuad(quad), editShader(shader), camera(camera) {};

    void render(
        int bfwidth,
        int bfheight,
        Framebuffer& framebuffer
    );

private:
    Screenquad& screenQuad;
    Shader& editShader;
    Camera& camera;
};