#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/Texture.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/core/SceneDrawer.h"

class SceneObjectPass
{

public:
    SceneObjectPass(
        SceneRenderResources& resources,
        ShadowResources& shadowResources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        LightSettings& lightSettings,
        Camera& camera,
        SceneDrawer& drawer
    ) : resources(resources),
        shadowResources(shadowResources),
        config(config),
        state(state),
        lightSettings(lightSettings),
        camera(camera),
        drawer(drawer)
    {
    }

    void renderNoLightingCube(int bfwidth, int bfheight);
    void renderNoLightingPlane(int bfwidth, int bfheight);
    void renderNoLightingMaterialSpheres(int bfwidth, int bfheight);
    void renderNoLightingModel(int bfwidth, int bfheight);

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    LightSettings& lightSettings;
    Camera& camera;
    SceneDrawer& drawer;

    void bindCubeTexture(Shader& shader, GLTexture& cubeTexture);
    void bindPlaneTexture(Shader& shader, GLTexture& floorTexture);
    void bindAlbedoTexture(Shader& shader, GLTexture& albedoTexture);

    Shader* getCubeShader();
    Shader* getPlaneShader();
    Shader* getModelShader();
};
    
