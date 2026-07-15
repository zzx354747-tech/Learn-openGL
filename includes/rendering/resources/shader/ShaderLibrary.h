#pragma once
#include "core/Shader.h"

struct ShaderLibrary
{
    Shader screen;
    Shader taa;
    Shader cubemap;
    Shader shadowDebug;
    Shader shadowMap;
    Shader pointShadowMap;
    Shader lightCube;
    Shader envCubemap;
    Shader irradiance;
    Shader prefilter;
    Shader brdf;
    Shader blur;
    Shader geometryPBR;
    Shader basicForward;
    Shader water;
    Shader lightingPass;
    Shader ssao;
    Shader ssaoBlur;

    ShaderLibrary();
};
