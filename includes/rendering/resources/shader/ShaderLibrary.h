#pragma once
#include "core/Shader.h"

struct ShaderLibrary
{
    Shader screen;
    Shader skyComposite;
    Shader cloudAcceleration;
    Shader volumetricLight;
    Shader volumetricLightComposite;
    Shader taa;
    Shader cubemap;
    Shader shadowDebug;
    Shader shadowMap;
    Shader cloudOpticalDepth;
    Shader cloudOpticalDepthBlur;
    Shader cloudOpticalDepthToTransmittance;
    Shader pointShadowMap;
    Shader lightCube;
    Shader envCubemap;
    Shader irradiance;
    Shader prefilter;
    Shader brdf;
    Shader blur;
    Shader geometryPBR;
    Shader vegetationGeometry;
    Shader vegetationShadow;
    Shader basicForward;
    Shader water;
    Shader waterCausticPhotons;
    Shader waterCausticBlur;
    Shader lightingPass;
    Shader ssao;
    Shader ssaoBlur;

    ShaderLibrary();
};
