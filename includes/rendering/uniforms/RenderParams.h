#pragma once
#include "core/Shader.h"

class RenderParams
{
public:
    bool  enableNormalMapping   = true;
    bool  enableParallaxMapping = false;
    float bumpNormalStrength    = 1.0f;
    int   numLayers             = 32;
    float parallaxHeightScale   = 0.1f;

    void apply(Shader& shader) const;
};