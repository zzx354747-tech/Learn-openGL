#include "rendering/passes/GeometryPass.h"

void GeometryPass::render(int bfwidth, int bfheight)
{
    gBuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCube(bfwidth, bfheight);
    renderPlane(bfwidth, bfheight);
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
        setupCubeMaterial(*shader);
        drawer.drawCubes(*shader);

        if (!resources.secondCubeDiffuseTexture||
            !resources.secondCubeNormalTexture||
            !resources.secondCubeParallaxTexture)
            return;

        bindCubeDiffuseTexture(*shader, *resources.secondCubeDiffuseTexture);
        bindCubeNormalTexture(*shader, *resources.secondCubeNormalTexture);
        bindCubeParallaxTexture(*shader, *resources.secondCubeParallaxTexture);
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

        bindPlaneTexture(*shader, *resources.floorTexture);
        setupPlaneMaterial(*shader);
        drawer.drawPlane(*shader);
}

void GeometryPass::renderModel(int bfwidth, int bfheight)
{
    Shader* shader = getGeometryShader();
    
        if (!shader)
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
    shader.setVec3("cameraPos", camera.Getposition());
}
    
Shader* GeometryPass::getGeometryShader()
{
    return resources.geometryShader;
}
