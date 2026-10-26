#include "rendering/passes/geometry/GeometryPass.h"
#include "scene/SceneObject.h"

GeometryPass::GeometryPass(
    SceneRenderResources& resources,
    SceneRenderConfig&    config,
    Camera&               camera,
    SphereDrawer&         sphereDrawer,
    ModelDrawer&          modelDrawer,
    GBuffer&              gBuffer,
    RenderParams&         renderParams,
    SceneObjectManager&   sceneObjectManager)
    : resources(resources)
    , config(config)
    , camera(camera)
    , sphereDrawer(sphereDrawer)
    , modelDrawer(modelDrawer)
    , gBuffer(gBuffer)
    , renderParams(renderParams)
    , sceneObjectManager(sceneObjectManager)
{}

void GeometryPass::render(int bfwidth, int bfheight)
{
    gBuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSpheres(bfwidth, bfheight);
    renderModels(bfwidth, bfheight);

    gBuffer.unbind();

    resources.registry.setTexture(
        resources.lightingHandles.gPosition,
        gBuffer.getPositionTexture()
    );
    resources.registry.setTexture(
        resources.lightingHandles.gNormalRoughness,
        gBuffer.getNormalRoughnessTexture()
    );
    resources.registry.setTexture(
        resources.lightingHandles.gAlbedoMetallic,
        gBuffer.getAlbedoMetallicTexture()
    );
}

void GeometryPass::renderSpheres(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();

    if (!shader)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);  // Owner A
    renderParams.apply(*shader);                                     // Owner B

    sceneObjectManager.drawObjects(*shader, config.sceneSelection, true, false, true);
}

void GeometryPass::renderModels(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();   // 复用同一份 GeometryPass shader

    if (!shader)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);  // Owner A
    renderParams.apply(*shader);                 // Owner B

    sceneObjectManager.drawObjects(*shader, config.sceneSelection, false, true);
}

Shader* GeometryPass::getPBRShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->geometryPBR : nullptr;
}
