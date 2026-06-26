#pragma once

#include "rendering/assets/EnvCubemap.h"
#include "rendering/postprocess/PingPong_Framebuffer.h"
#include "rendering/assets/IrradianceMap.h"
#include "rendering/assets/PrefilterMap.h"
#include "rendering/assets/BrdfLUT.h"

struct SceneRenderOutputResources
{
    EnvCubemap* skybox = nullptr;
    IrradianceMap* irradianceMap = nullptr;
    PrefilterMap* prefilterMap = nullptr;
    BrdfLUT* brdfLUT = nullptr;
    PingPongFramebuffer* pingpongFBO = nullptr;
};
