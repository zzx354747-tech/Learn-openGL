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
#include "rendering/postprocess/SpotShadowMap.h"
#include "rendering/postprocess/PingPong_Framebuffer.h"
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
    Shader* basicModelShader = nullptr;
    Shader* lightingModelShader = nullptr;
    Shader* reflectModelShader = nullptr;
    Shader* blurShader = nullptr;
    Shader* geometryShader = nullptr;
    Shader* lightingPassShader = nullptr;

    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    LightMesh* lightMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;
    Model* model = nullptr;

    CubeMap* skybox = nullptr;
    PingPongFramebuffer* pingpongFBO = nullptr;

    GLTexture* floorTexture = nullptr;
    GLTexture* cubeDiffuseTexture = nullptr;
    GLTexture* cubeNormalTexture = nullptr;
    GLTexture* cubeParallaxTexture = nullptr;
    GLTexture* secondCubeDiffuseTexture = nullptr;
    GLTexture* secondCubeNormalTexture = nullptr;
    GLTexture* secondCubeParallaxTexture = nullptr;
};

struct ShadowResources
{
    DirectionalShadowMap* shadowMap = nullptr;
    PointShadowMap* pointShadowMap = nullptr;
    SpotShadowMap* spotShadowMap = nullptr;
};
