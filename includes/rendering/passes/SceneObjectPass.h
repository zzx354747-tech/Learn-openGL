#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/Texture.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/core/SceneDrawer.h"

class SceneObjectPass
{

public:
    SceneObjectPass(
        SceneRenderResources& resources,
        ShadowResources& shadowResources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        RenderMode& renderMode,
        LightSettings& lightSettings,
        Camera& camera,
        SceneDrawer& drawer
    ) : resources(resources),
        shadowResources(shadowResources),
        config(config),
        state(state),
        renderMode(renderMode),
        lightSettings(lightSettings),
        camera(camera),
        drawer(drawer)
    {
    }

    void renderCube(int bfwidth, int bfheight);
    void renderPlane(int bfwidth, int bfheight);
    void renderModel(Model& model, int bfwidth, int bfheight);

private:
    SceneRenderResources& resources;
    ShadowResources& shadowResources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    RenderMode& renderMode;
    LightSettings& lightSettings;
    Camera& camera;
    SceneDrawer& drawer;

    void bindCubeTexture(Shader& shader, GLTexture& cubeTexture);
    void bindPlaneTexture(Shader& shader, GLTexture& floorTexture);

    void setupObjectLighting(Shader& shader);
    void setupPointShadow(Shader& shader, unsigned int textureUnit = 2);

    Shader* getCubeShader();
    Shader* getPlaneShader();
    Shader* getModelShader();

};
    
