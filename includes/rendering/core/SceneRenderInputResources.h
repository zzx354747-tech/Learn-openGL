#pragma once

#include "core/Shader.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/SphereMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/Model/Model.h"
#include "rendering/assets/Texture.h"
#include "rendering/core/SceneRenderTypes.h"

struct PBRMaterialTextures
{
    GLTexture* albedo = nullptr;
    GLTexture* normal = nullptr;
    GLTexture* roughness = nullptr;
    GLTexture* metallic = nullptr;
    GLTexture* displacement = nullptr;

    bool isValid() const
    {
        return albedo && roughness && metallic;
    }
};

struct SceneRenderInputResources
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
    Shader* blurShader = nullptr;
    Shader* geometryShader = nullptr;
    Shader* lightingPassShader = nullptr;
    Shader* ssaoShader = nullptr;
    Shader* ssaoBlurShader = nullptr;
    Shader* envCubemapShader = nullptr;
    Shader* prefilterShader = nullptr;
    Shader* irradianceShader = nullptr;
    Shader* brdfLUTShader = nullptr;

    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SphereMesh* sphereMesh = nullptr;
    LightMesh* lightMesh = nullptr;
    SkyboxMesh* skyboxMesh = nullptr;
    Model* model = nullptr;

    GLTexture* floorTexture = nullptr;
    GLTexture* cubeDiffuseTexture = nullptr;
    GLTexture* cubeNormalTexture = nullptr;
    GLTexture* cubeParallaxTexture = nullptr;
    GLTexture* secondCubeDiffuseTexture = nullptr;
    GLTexture* secondCubeNormalTexture = nullptr;
    GLTexture* secondCubeParallaxTexture = nullptr;
    GLTexture* clearSphereAlbedoTexture = nullptr;
    GLTexture* defaultRoughnessTexture = nullptr;
    GLTexture* defaultMetallicTexture = nullptr;

    PBRMaterialTextures floorPBRMaterial;
    PBRMaterialTextures materialSpherePBRMaterials[MaterialSphereCount];
};
