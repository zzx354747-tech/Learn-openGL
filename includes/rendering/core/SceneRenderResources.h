#pragma once

#include "core/Shader.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/Model/Model.h"
#include "rendering/assets/CubeMap.h"
#include "rendering/assets/Texture.h"
#include "rendering/postprocess/DirectionalShadowMap.h"
#include "rendering/postprocess/PointShadowMap.h"
#include "rendering/core/SceneRenderTypes.h"

struct SceneRenderResources
{
   Shader* basicCubeShader = nullptr;
    Shader* basicPlaneShader = nullptr;
    Shader* lightingCubeShader = nullptr;
    Shader* lightingPlaneShader = nullptr;
    Shader* lightCubeShader = nullptr;
    Shader* reflectShader = nullptr;
    Shader* shadowDebugShader = nullptr;
    Shader* shadowMapShader = nullptr;
    Shader* pointShadowMapShader = nullptr;
    Shader* modelShader = nullptr;

    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    LightMesh* lightMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;
    Model* model = nullptr;

    CubeMap* skybox = nullptr;

    GLTexture* floorTexture = nullptr;
    GLTexture* cubeTexture = nullptr;
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
    PointShadowMap* pointShadowMap = nullptr;
};
