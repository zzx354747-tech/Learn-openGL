#pragma once

#include "rendering/core/SceneRenderInputResources.h"
#include "rendering/core/SceneRenderOutputResources.h"
#include "rendering/postprocess/DirectionalShadowMap.h"
#include "rendering/postprocess/PointShadowMap.h"
#include "rendering/postprocess/SpotShadowMap.h"

struct SceneRenderResources : SceneRenderInputResources, SceneRenderOutputResources
{
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
    PointShadowMap* pointShadowMap = nullptr;
    SpotShadowMap* spotShadowMap = nullptr;
};
