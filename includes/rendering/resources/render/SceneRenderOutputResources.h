#pragma once

#include "rendering/assets/ibl/BrdfLUT.h"
#include "rendering/assets/texture/EnvCubemap.h"
#include "rendering/assets/ibl/IrradianceMap.h"
#include "rendering/assets/ibl/PrefilterMap.h"

struct SceneRenderOutputResources
{
    EnvCubemap* skybox = nullptr;
    IrradianceMap* irradianceMap = nullptr;
    PrefilterMap* prefilterMap = nullptr;
    BrdfLUT* brdfLUT = nullptr;
};
