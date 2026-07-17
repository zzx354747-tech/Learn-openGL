#include "rendering/passes/geometry/GeometryPass.h"
#include "rendering/assets/mesh/AlpineVegetationSystem.h"

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
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthRange(0.0, 1.0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
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
    resources.registry.setTexture(
        resources.lightingHandles.gVelocity,
        gBuffer.getVelocityTexture()
    );
}

void GeometryPass::renderSpheres(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();
    if (!shader)
        return;

    shader->use();
    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);
    renderParams.apply(*shader);
    sphereDrawer.draw(*shader);
}

void GeometryPass::renderModels(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();
    if (!shader)
        return;

    shader->use();
    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);
    renderParams.apply(*shader);
    shader->setFloat("u_sunAzimuth", config.terrainSunAzimuth);
    shader->setFloat("u_terrainSunHeightShift", config.terrainSunHeightShift);
    shader->setFloat("u_terrainNoiseHeightShift", config.terrainNoiseHeightShift);
    shader->setFloat("u_grassEnd", config.terrainGrassEnd);
    shader->setFloat("u_rockStart", config.terrainRockStart);
    shader->setFloat("u_snowStart", config.terrainSnowStart);
    shader->setFloat("u_snowEnd", config.terrainSnowEnd);
    shader->setFloat("u_terrainBlendSharpness", config.terrainBlendSharpness);
    shader->setFloat("u_terrainTextureScale", config.terrainTextureScale);
    shader->setInt("u_terrainDebugMode", config.terrainDebugMode);

    const bool alpineVegetation =
        config.sceneSelection == SceneSelection::FujiTerrain &&
        config.enableVegetation && resources.vegetationSystem;
    if (alpineVegetation)
        resources.vegetationSystem->bindTerrainDensity(*shader);
    else
        shader->setBool("hasVegetationDensity", false);

    // Populate terrain depth before drawing vegetation. This both guarantees
    // mountain occlusion and enables early depth rejection for hidden foliage.
    modelDrawer.draw(*shader);

    if (alpineVegetation && resources.shaderLibrary)
    {
        // Restate the full depth contract instead of inheriting state from a
        // previous pass: foliage writes depth, and fragments behind terrain
        // fail the fixed-function depth test before reaching the GBuffer.
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthRange(0.0, 1.0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        resources.vegetationSystem->drawGeometry(
            resources.shaderLibrary->vegetationGeometry, camera, config);
    }
}

Shader* GeometryPass::getPBRShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->geometryPBR : nullptr;
}
