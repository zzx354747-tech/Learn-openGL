#include "rendering/passes/GeometryPass.h"

void GeometryPass::render(int bfwidth, int bfheight)
{
    gBuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderPlane(bfwidth, bfheight);
    renderMaterialSpheres(bfwidth, bfheight);
    renderClearSphere(bfwidth, bfheight);
    renderModel(bfwidth, bfheight);
    gBuffer.unbind();
}

void GeometryPass::renderCube(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();
        
        if (!shader || 
            !resources.cubeMesh || 
            !resources.cubeDiffuseTexture||
            !resources.cubeNormalTexture||
            !resources.cubeParallaxTexture)
            return;
        shader->use();

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        bindCubeDiffuseTexture(*shader, *resources.cubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.cubeNormalTexture);
        bindCubeParallaxTexture(*shader, *resources.cubeParallaxTexture);
        bindDefaultPBRFallbackTextures(*shader);
        setupCubeMaterial(*shader);
        drawer.drawCubes(*shader);

        if (!resources.secondCubeDiffuseTexture||
            !resources.secondCubeNormalTexture||
            !resources.secondCubeParallaxTexture)
            return;

        bindCubeDiffuseTexture(*shader, *resources.secondCubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.secondCubeNormalTexture);
        bindCubeParallaxTexture(*shader, *resources.secondCubeParallaxTexture);
        bindDefaultPBRFallbackTextures(*shader);
        setupCubeMaterial(*shader);
        drawer.drawSecondCubes(*shader);
}

void GeometryPass::renderPlane(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();
    
        if (!shader ||
            !resources.planeMesh||
            !resources.floorTexture||
            !config.enableFloor)
            return;

        shader->use();

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        if (resources.floorPBRMaterial.isValid())
        {
            bindPBRMaterial(*shader, resources.floorPBRMaterial);
            setupPBRMaterial(
                *shader,
                resources.floorPBRMaterial,
                config.floorEnableNormalMapping,
                config.floorEnableParallaxMapping,
                config.floorParallaxHeightScale,
                config.floorNumLayers,
                config.floorBumpNormalStrength);
        }
        else
        {
            bindPlaneTexture(*shader, *resources.floorTexture);
            setupPlaneMaterial(*shader);
        }

        drawer.drawPlane(*shader);
}

void GeometryPass::renderMaterialSpheres(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();

    if (!shader || !resources.sphereMesh)
        return;

    shader->use();
    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        const PBRMaterialTextures& material = resources.materialSpherePBRMaterials[i];
        if (!material.isValid())
            continue;

        bindPBRMaterial(*shader, material);
        setupPBRMaterial(
            *shader,
            material,
            config.modelEnableNormalMapping,
            config.modelEnableParallaxMapping,
            config.modelParallaxHeightScale,
            config.modelNumLayers,
            config.materialSphereBumpNormalStrength);
        drawer.drawMaterialSphere(*shader, i);
    }
}

void GeometryPass::renderClearSphere(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();

    if (!shader ||
        !resources.sphereMesh ||
        !resources.clearSphereAlbedoTexture ||
        !config.enableClearSphere)
        return;

    shader->use();
    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

    resources.clearSphereAlbedoTexture->bind(0);
    shader->setInt("albedoTexture", 0);
    shader->setInt("diffuseTexture", 0);
    bindDefaultPBRFallbackTextures(*shader);
    setupClearSphereMaterial(*shader);
    drawer.drawClearSphere(*shader);
}

void GeometryPass::renderModel(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();
    
        if (!shader || !config.enableModel)
            return;

        shader->use();

        CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);

        setupModelMaterial(*shader);
        drawer.drawModel(*shader);
}

void GeometryPass::bindCubeDiffuseTexture(Shader& shader, GLTexture& cubeTexture)
{
    cubeTexture.bind(0);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("albedoTexture", 0);
}

void GeometryPass::bindCubeNormalTexture(Shader& shader, GLTexture& cubeTexture)
{
    cubeTexture.bind(1);
    shader.setInt("normalTexture", 1);
}

void GeometryPass::bindCubeParallaxTexture(Shader& shader, GLTexture& cubeTexture)
{
    cubeTexture.bind(2);
    shader.setInt("parallaxTexture", 2);
}  

void GeometryPass::bindPlaneTexture(Shader& shader, GLTexture& floorTexture)
{
    floorTexture.bind(0);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("albedoTexture", 0);
}

void GeometryPass::bindPBRMaterial(Shader& shader, const PBRMaterialTextures& material)
{
    material.albedo->bind(0);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("albedoTexture", 0);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", true);
    shader.setBool("hasRoughnessMap", true);
    shader.setVec4("baseColorFactor", glm::vec4(1.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 1.0f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);

    if (material.normal)
    {
        material.normal->bind(1);
        shader.setInt("normalTexture", 1);
    }

    if (material.displacement)
    {
        material.displacement->bind(2);
        shader.setInt("parallaxTexture", 2);
    }

    material.metallic->bind(3);
    shader.setInt("metallicTexture", 3);

    material.roughness->bind(4);
    shader.setInt("roughnessTexture", 4);
}

void GeometryPass::bindDefaultPBRFallbackTextures(Shader& shader)
{
    if (resources.defaultMetallicTexture)
    {
        resources.defaultMetallicTexture->bind(3);
        shader.setInt("metallicTexture", 3);
    }

    if (resources.defaultRoughnessTexture)
    {
        resources.defaultRoughnessTexture->bind(4);
        shader.setInt("roughnessTexture", 4);
    }
}

void GeometryPass::setupCubeMaterial(Shader& shader)
{
    shader.setFloat("parallaxHeightScale", config.cubeParallaxHeightScale);
    shader.setFloat("heightScale", config.cubeParallaxHeightScale);
    shader.setFloat("bumpNormalStrength", 1.0f);
    shader.setInt("numLayers", config.cubeNumLayers);
    shader.setBool("enableNormalMapping", config.cubeEnableNormalMapping);
    shader.setBool("enableParallaxMapping", config.cubeEnableParallaxMapping);
    shader.setBool("hasNormalMap", true);
    shader.setBool("hasParallaxMap", true);
    shader.setBool("hasSpecularMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", true);
    shader.setBool("hasRoughnessMap", true);
    shader.setVec4("baseColorFactor", glm::vec4(1.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 1.0f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setVec3("cameraPos", camera.Getposition());
}

void GeometryPass::setupPlaneMaterial(Shader& shader)
{
    shader.setBool("enableNormalMapping", false);
    shader.setBool("enableParallaxMapping", false);
    shader.setBool("hasNormalMap", false);
    shader.setBool("hasParallaxMap", false);
    shader.setFloat("bumpNormalStrength", 1.0f);
    shader.setBool("hasSpecularMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", false);
    shader.setBool("hasRoughnessMap", false);
    shader.setVec4("baseColorFactor", glm::vec4(1.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 1.0f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setVec3("cameraPos", camera.Getposition());
}

void GeometryPass::setupClearSphereMaterial(Shader& shader)
{
    shader.setBool("enableNormalMapping", false);
    shader.setBool("enableParallaxMapping", false);
    shader.setBool("hasNormalMap", false);
    shader.setBool("hasParallaxMap", false);
    shader.setBool("hasSpecularMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", false);
    shader.setBool("hasRoughnessMap", false);
    shader.setVec4("baseColorFactor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 0.04f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setFloat("parallaxHeightScale", 0.0f);
    shader.setFloat("heightScale", 0.0f);
    shader.setFloat("bumpNormalStrength", 1.0f);
    shader.setInt("numLayers", 1);
    shader.setVec3("cameraPos", camera.Getposition());
}

void GeometryPass::setupPBRMaterial(
    Shader& shader,
    const PBRMaterialTextures& material,
    bool enableNormalMapping,
    bool enableParallaxMapping,
    float parallaxHeightScale,
    int numLayers,
    float bumpNormalStrength)
{
    shader.setFloat("parallaxHeightScale", parallaxHeightScale);
    shader.setFloat("heightScale", parallaxHeightScale);
    shader.setFloat("bumpNormalStrength", bumpNormalStrength);
    shader.setInt("numLayers", numLayers);
    shader.setBool("enableNormalMapping", enableNormalMapping);
    shader.setBool("enableParallaxMapping", enableParallaxMapping);
    shader.setBool("hasNormalMap", material.normal != nullptr);
    shader.setBool("hasParallaxMap", material.displacement != nullptr);
    shader.setBool("hasSpecularMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", true);
    shader.setBool("hasRoughnessMap", true);
    shader.setVec4("baseColorFactor", glm::vec4(1.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 1.0f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setVec3("cameraPos", camera.Getposition());
}

void GeometryPass::setupModelMaterial(Shader& shader)
{
    shader.setFloat("parallaxHeightScale", config.modelParallaxHeightScale);
    shader.setFloat("heightScale", config.modelParallaxHeightScale);
    shader.setFloat("bumpNormalStrength", config.modelBumpNormalStrength);
    shader.setInt("numLayers", config.modelNumLayers);
    shader.setBool("enableNormalMapping", config.modelEnableNormalMapping);
    shader.setBool("enableParallaxMapping", config.modelEnableParallaxMapping);
    shader.setBool("hasNormalMap", false);
    shader.setBool("hasParallaxMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasMetallicMap", false);
    shader.setBool("hasRoughnessMap", false);
    shader.setVec4("baseColorFactor", glm::vec4(1.0f));
    shader.setFloat("metallicFactor", 0.0f);
    shader.setFloat("roughnessFactor", 1.0f);
    shader.setBool("alphaMask", false);
    shader.setFloat("alphaCutoff", 0.5f);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setVec3("cameraPos", camera.Getposition());
}
    
Shader* GeometryPass::getGeometryShader()
{
    return resources.geometryShader;
}
