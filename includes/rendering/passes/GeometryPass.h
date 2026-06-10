#pragma once
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/core/SceneRenderTypes.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/postprocess/Gbuffer.h"
#include "rendering/uniforms/CameraUniformSetter.h"

class GeometryPass
{
public:
    GeometryPass(
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        Camera& camera,
        SceneDrawer& drawer,
        GBuffer& gBuffer
    ) : resources(resources),
        config(config),
        state(state),
        camera(camera),
        drawer(drawer),
        gBuffer(gBuffer)
    {}

    void render(int bfwidth, int bfheight);

    void renderCube(int bfwidth, int bfheight);
    void renderPlane(int bfwidth, int bfheight);
    void renderModel(int bfwidth, int bfheight);

private:
    SceneRenderResources& resources;
    SceneRenderConfig& config;
    SceneRenderState& state;
    Camera& camera;
    SceneDrawer& drawer;
    GBuffer& gBuffer;

    void bindCubeDiffuseTexture(Shader& shader, GLTexture& cubeTexture);
    void bindCubeNormalTexture(Shader& shader, GLTexture& cubeTexture);
    void bindCubeParallaxTexture(Shader& shader, GLTexture& cubeTexture);
    void bindPlaneTexture(Shader& shader, GLTexture& floorTexture);

    void setupCubeMaterial(Shader& shader);
    void setupPlaneMaterial(Shader& shader);
    void setupModelMaterial(Shader& shader);

    Shader* getGeometryShader();
};