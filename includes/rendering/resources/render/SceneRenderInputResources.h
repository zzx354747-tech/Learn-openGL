#pragma once

#include "rendering/assets/mesh/CubeMesh.h"
#include "rendering/assets/mesh/PlaneMesh.h"
#include "rendering/assets/mesh/SphereMesh.h"
#include "rendering/assets/mesh/LightMesh.h"
#include "rendering/assets/mesh/SkyboxMesh.h"
#include "rendering/modelload/Model.h"
#include "rendering/assets/texture/Texture.h"
#include "rendering/resources/render/SceneRenderTypes.h"

struct SceneRenderInputResources
{
    LightMesh* lightMesh = nullptr;
    SphereMesh* sphereMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;
};
