#pragma once

#include "core/Shader.h"
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
    Shader* lightCubeShader = nullptr;
    Shader* reflectShader = nullptr;
    Shader* shadowDebugShader = nullptr;
    Shader* shadowMapShader = nullptr;
    Shader* pointShadowMapShader = nullptr;
    Shader* blurShader = nullptr;
    Shader* geometryPBRShader = nullptr;
    Shader* lightingPassShader = nullptr;
    Shader* ssaoShader = nullptr;
    Shader* ssaoBlurShader = nullptr;
    Shader* envCubemapShader = nullptr;

    LightMesh* lightMesh = nullptr;
    SphereMesh* sphereMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;
};
