// rendering/core/RendererScene.cpp
#include "rendering/core/RendererScene.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

RendererScene::RendererScene(int width, int height)
    : fb(width, height)
    , sceneGBuffer(width, height)
    , sceneSSAO(width, height)
    , shadowMap(4096, 4096)
    , pointShadowMap(1024, 1024, 1.0f, 50.0f)
    , spotShadowMap(1024, 1024, 1.0f, 50.0f)
    , sceneConfig(makeLivingRoomPreset())
    , sphereDrawer(&sphereMesh, &sceneState, &sceneConfig)
    , livingRoomDrawer("../3D_model/living_room_interior_free.glb", &sceneConfig)
    , sceneObjectManager(&livingRoomDrawer, &sphereDrawer)
    , objectPickPass(width, height)
    , brightPrefilterPass(width, height, shaderLibrary.brightPrefilter, screenQuad)
    , pointShadowPass(pointShadowMap,
                    shaderLibrary.pointShadowMap,
                    sphereDrawer,
                    livingRoomDrawer,
                    sceneResources.registry,
                    sceneResources.lightingHandles.depthCubeMap)
    , directionalShadowPass(shadowMap,
                    shaderLibrary.shadowMap,
                    sphereDrawer,
                    livingRoomDrawer,
                    sceneState,
                    lightSettings,
                    sceneResources.registry,
                    sceneResources.lightingHandles.shadowMap)
    , spotShadowPass(spotShadowMap,
                    shaderLibrary.shadowMap,
                    sphereDrawer,
                    livingRoomDrawer,
                    camera,
                    sceneState,
                    lightSettings,
                    sceneResources.registry,
                    sceneResources.lightingHandles.spotShadowMap),
                    brdfLUT(shaderLibrary.brdf),
                    environmentController(sceneConfig,
                    renderParams,
                    lightSettings,
                    sceneResources,
                    shaderLibrary.irradiance,
                    shaderLibrary.prefilter)
    , geometryPass(sceneResources,
                    sceneConfig,
                    camera,
                    sphereDrawer,
                    livingRoomDrawer,
                    sceneGBuffer,
                    renderParams,
                    sceneObjectManager)
    , lightingPass(sceneResources,
                    shadowResources,
                    sceneConfig,
                    sceneState,
                    lightSettings,
                    camera)

    , bloomPass(width, height,
                    shaderLibrary.downSample,
                    shaderLibrary.upSample,
                    screenQuad)

    , ssaoCommonPass(sceneResources,
                    sceneSSAO,
                    screenQuad,
                    camera,
                    sceneGBuffer)
    , sceneRender(camera,
                    shadowResources,
                    sceneResources,
                    sceneConfig,
                    sceneState,
                    lightSettings,
                    directionalShadowPass,
                    brightPrefilterPass,
                    bloomPass,
                    pointShadowPass,
                    spotShadowPass,
                    geometryPass,
                    lightingPass,
                    sceneGBuffer,
                    ssaoCommonPass,
                    sphereDrawer,
                    livingRoomDrawer,
                    screenQuad,
                    shaderLibrary.editLine)

    , renderModeIndex(0)
    , environmentIndex(getEnvironmentIndex(sceneConfig.environmentSelection))
    , uiState{
                    renderModeIndex,
                    environmentIndex,
                    sceneConfig,
                    renderParams,
                    lightSettings,
                    false,
                    [this]() { return environmentController.load(); },
                    [this]() { environmentController.applyPreset(); }
    }
{
    sceneResources.shaderLibrary = &shaderLibrary;
    sceneResources.sphereMesh = &sphereMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;

    sphereDrawer.loadMaterials("../textures/PBR/");

    livingRoomDrawer.setVisibleInScene(SceneSelection::LivingRoom);
    sceneObjectManager.addSceneObject();
    glm::mat4 livingRoomTransform(1.0f);
    livingRoomTransform = glm::translate(livingRoomTransform, glm::vec3(0.0f, 0.0f, -3.0f));
    livingRoomTransform = glm::scale(livingRoomTransform, glm::vec3(0.2f));
    livingRoomDrawer.setTransform(livingRoomTransform);

    shadowResources.shadowMap = &shadowMap;
    shadowResources.pointShadowMap = &pointShadowMap;
    shadowResources.spotShadowMap = &spotShadowMap;

    sceneResources.registry.setTexture(
        sceneResources.lightingHandles.brdfLUT,
        brdfLUT.GetID());

    sceneResources.brdfLUT = &brdfLUT;
    environmentController.load();
    environmentController.applyPreset();
}

void RendererScene::resize(int width, int height)
{
    fb.resize(width, height);
    sceneGBuffer.resize(width, height);
    objectPickPass.resize(width, height);
    sceneSSAO.resize(width, height);
    brightPrefilterPass.resize(width, height);
    bloomPass.resize(width, height);
}

void RendererScene::render(int width, int height)
{
    sceneRender.render(
        width,
        height,
        shaderLibrary.screen,
        screenQuad,
        fb
    );
}

void RendererScene::renderUI(float fps, float swapWaitMs)
{
   sceneRenderUI.renderUI(
        uiState,
        sceneObjectManager,
        selectedObjectID,
        fps,
        swapWaitMs
    );
}

void RendererScene::toggleEditMode()
{
    sceneConfig.enableEdit = !sceneConfig.enableEdit;
}

bool RendererScene::getEditMode() const
{
    return sceneConfig.enableEdit;
}

Camera& RendererScene::getCamera()
{
    return camera;
}

SceneObjectID RendererScene::selectObjectAtPixel(
    int pixelX,
    int pixelY,
    int framebufferWidth,
    int framebufferHeight)
{
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return selectedObjectID;

    if (pixelX < 0 || pixelX >= framebufferWidth ||
        pixelY < 0 || pixelY >= framebufferHeight)
    {
        return selectedObjectID;
    }

    objectPickPass.resize(
        framebufferWidth,
        framebufferHeight
    );

    objectPickPass.begin();

    // Restore picking state even if drawing exits through an exception.
    struct PickStateGuard
    {
        ObjectPickPass& pass;
        ~PickStateGuard() { pass.end(); }
    } pickStateGuard{objectPickPass};

    Shader& shader = shaderLibrary.objectPick;
    shader.use();

    CameraUniformSetter::apply(
        shader,
        camera,
        framebufferWidth,
        framebufferHeight
    );

    for (const SceneObject& object :
         sceneObjectManager.getSceneObjects())
    {
        if (!object.isVisible ||
            object.scene != sceneConfig.sceneSelection)
        {
            continue;
        }

        shader.setUInt("objectID", object.id);

        const glm::mat4 matrix = object.transform.matrix();

        if (const auto* sphereRef =
            std::get_if<sphereRenderRef>(&object.ref))
        {
            if (sphereRef->drawer)
            {
                sphereRef->drawer->drawPick(shader, matrix);
            }
        }
        else if (const auto* modelRef =
                 std::get_if<ModelDrawer*>(&object.ref))
        {
            if (*modelRef)
            {
                (*modelRef)->draw(shader, matrix);
            }
        }
    }

    selectedObjectID =
        objectPickPass.readID(pixelX, pixelY);

    objectPickPass.end();

    if (SceneObject* object =
        sceneObjectManager.getSceneObject(selectedObjectID))
    {
        std::cout
            << "Selected: " << object->name
            << ", ID: " << object->id
            << '\n';
    }
    else
    {
        std::cout << "Selection cleared\n";
    }

    return selectedObjectID;
}
