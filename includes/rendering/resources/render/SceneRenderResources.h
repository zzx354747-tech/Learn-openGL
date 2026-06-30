#pragma once

#include "rendering/resources/render/SceneRenderInputResources.h"
#include "rendering/resources/render/SceneRenderOutputResources.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/resources/shadow/PointShadowMap.h"
#include "rendering/resources/shadow/SpotShadowMap.h"

#include "rendering/core/ResourceRegistry.h"
#include "rendering/passes/lighting/LightingPassResourceHandles.h"

struct SceneRenderResources : SceneRenderInputResources, SceneRenderOutputResources
{
    ResourceRegistry registry;
    LightingPassResourceHandles lightingHandles;

    void declareLightingPassResources();
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
    PointShadowMap* pointShadowMap = nullptr;
    SpotShadowMap* spotShadowMap = nullptr;
};
