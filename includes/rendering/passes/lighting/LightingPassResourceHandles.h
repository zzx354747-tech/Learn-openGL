#pragma once
#include "rendering/core/ResourceRegistry.h"

struct LightingPassResourceHandles
{
    ResourceHandle gPosition = 0;
    ResourceHandle gNormalRoughness = 0;
    ResourceHandle gAlbedoMetallic = 0;
    ResourceHandle gVelocity = 0;
    ResourceHandle ao = 0;
    ResourceHandle shadowMap = 0;
    ResourceHandle depthCubeMap = 0;
    ResourceHandle spotShadowMap = 0;
    ResourceHandle brdfLUT = 0;
    ResourceHandle irradianceMap = 0;
    ResourceHandle prefilterMap = 0;
};
