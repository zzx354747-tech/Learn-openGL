#include "rendering/passes/geometry/GeometryPass.h"

GeometryPass::GeometryPass(
    SceneRenderResources& resources,
    SceneRenderConfig&    config,
    Camera&               camera,
    SphereDrawer&         sphereDrawer,
    ModelDrawer&          modelDrawer,
    GBuffer&              gBuffer,
    RenderParams&         renderParams)
    : resources(resources)
    , config(config)
    , camera(camera)
    , sphereDrawer(sphereDrawer)
    , modelDrawer(modelDrawer)
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

    sphereDrawer.draw(*shader);
}

void GeometryPass::renderModels(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();   // 复用同一份 GeometryPass shader

    if (!shader)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);  // Owner A
    renderParams.apply(*shader);                                     // Owner B
    shader->setFloat("u_sunAzimuth", config.terrainSunAzimuth);
    shader->setFloat("u_terrainSunHeightShift", config.terrainSunHeightShift);
    shader->setFloat("u_terrainNoiseHeightShift", config.terrainNoiseHeightShift);
    shader->setFloat("u_grassEnd", config.terrainGrassEnd);
    shader->setFloat("u_rockStart", config.terrainRockStart);
    shader->setFloat("u_snowStart", config.terrainSnowStart);
    shader->setFloat("u_snowEnd", config.terrainSnowEnd);
    shader->setFloat("u_steepRockStart", config.terrainSteepRockStart);
    shader->setFloat("u_steepRockEnd", config.terrainSteepRockEnd);
    shader->setFloat("u_snowSlopeStart", config.terrainSnowSlopeStart);
    shader->setFloat("u_snowSlopeEnd", config.terrainSnowSlopeEnd);
    shader->setFloat("u_terrainBlendSharpness", config.terrainBlendSharpness);
    shader->setFloat("u_terrainTextureScale", config.terrainTextureScale);
    shader->setInt("u_terrainDebugMode", config.terrainDebugMode);

    modelDrawer.draw(*shader);   // Owner C：model 矩阵在这里面设置
}

Shader* GeometryPass::getPBRShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->geometryPBR : nullptr;
}
