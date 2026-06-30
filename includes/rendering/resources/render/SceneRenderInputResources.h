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

struct PBRMaterialTextures
{
    GLTexture* albedo = nullptr;
    GLTexture* normal = nullptr;
    GLTexture* roughness = nullptr;
    GLTexture* metallic = nullptr;
    GLTexture* displacement = nullptr;

    bool isValid() const;
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
