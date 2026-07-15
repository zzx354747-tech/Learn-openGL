#pragma once

#include "rendering/assets/ibl/BrdfLUT.h"
#include "rendering/assets/texture/EnvCubemap.h"
#include "rendering/assets/ibl/IrradianceMap.h"
#include "rendering/assets/ibl/PrefilterMap.h"
#include "rendering/resources/framebuffer/PingPong_Framebuffer.h"

struct SceneRenderOutputResources
{
    EnvCubemap* skybox = nullptr;
    unsigned int sunTexture = 0;
    IrradianceMap* irradianceMap = nullptr;
    PrefilterMap* prefilterMap = nullptr;
    BrdfLUT* brdfLUT = nullptr;
    PingPongFramebuffer* pingpongFBO = nullptr;
};
