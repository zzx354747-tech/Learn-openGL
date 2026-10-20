#include "rendering/passes/geometry/GeometryPass.h"
#include "scene/sceneObject.h"

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

     // 传递sphere矩阵
    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        SceneObject* object = sceneObjectManager.getSceneObject(i + 1);  // 获取小球对象
        // 若该小球对象不可用，则跳过处理下一个小球对象
        if (!object || !object->isVisible)
            continue;

        // 读取object对象中的ref成员，并根据它是否为sphereRenderRef类型来决定是否继续处理
        // 此为variant有关的语法
        auto* sphereRef = std::get_if<sphereRenderRef>(&object->ref);
        if (!sphereRef || !sphereRef->drawer)
            continue;
        
        // 传递小球的模型矩阵
        glm::mat4 modelMatrix = object->transform.matrix();
        shader->setMat4("model", modelMatrix);

        sphereRef->drawer->drawInstance(*shader, modelMatrix, sphereRef->materialIndex);
    }
}

void GeometryPass::renderModels(int bfwidth, int bfheight)
{
    Shader* shader = getPBRShader();   // 复用同一份 GeometryPass shader

    if (!shader)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);  // Owner A
    renderParams.apply(*shader);                 // Owner B

    SceneObject* object =
    sceneObjectManager.getSceneObject(MaterialSphereCount + 1);

    if (!object ||
        !object->isVisible ||
        object->scene != config.sceneSelection)
    {
        return;
    }

    modelDrawer.draw(*shader, object->transform.matrix());
}

Shader* GeometryPass::getPBRShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->geometryPBR : nullptr;
}
