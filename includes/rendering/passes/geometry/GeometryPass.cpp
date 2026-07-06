#include "rendering/passes/geometry/GeometryPass.h"

GeometryPass::GeometryPass(
    SceneRenderResources& resources,
    SceneRenderConfig&    config,
    Camera&               camera,
    SphereDrawer&         sphereDrawer,
    GBuffer&              gBuffer,
    RenderParams&         renderParams)
    : resources(resources)
    , config(config)
    , camera(camera)
    , sphereDrawer(sphereDrawer)
    , gBuffer(gBuffer)
    , renderParams(renderParams)
{}

void GeometryPass::render(int bfwidth, int bfheight)
{
    gBuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSpheres(bfwidth, bfheight);

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

    sphereDrawer.draw(*shader);
}

Shader* GeometryPass::getPBRShader()
{
    return resources.geometryPBRShader;
}
