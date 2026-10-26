// rendering/core/RendererScene.cpp
#include "rendering/core/RendererScene.h"
#include "scene/SceneSaver.h"
#include "scene/SceneLoder.h"
#include <algorithm>
#include <filesystem>
#include <exception>
#include <glm/gtc/matrix_transform.hpp>

RendererScene::RendererScene(int width, int height)
    : fb(width, height)
    , editorOutput(width, height)
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
    , outlinePass(fb, sceneGBuffer, shaderLibrary.outlineMask, shaderLibrary.outlineScreen,
                  camera, screenQuad, width, height)
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
                    shaderLibrary.editLine,
                    outlinePass,
                    sceneObjectManager)

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
    viewportPixelWidth = width;
    viewportPixelHeight = height;
    uiState.queueModelImport = [this](const std::string& path) { queueModelImport(path); };
    uiState.saveSceneToFile = [this]() { return saveSceneToFile(); };
    uiState.loadSceneFromFile = [this]() { return loadSceneFromFile(); };
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

bool RendererScene::saveSceneToFile()
{
    try
    {
        return saveScene(
            sceneObjectManager,
            lightSettings,
            sceneConfig,
            renderParams,
            sceneState,
            camera);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to save scene: " << error.what() << '\n';
        return false;
    }
}

bool RendererScene::loadSceneFromFile()
{
    // The current loader validates camera data but does not restore scene members yet.
    return loadScene(camera);
}

void RendererScene::resize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    viewportPixelWidth = width;
    viewportPixelHeight = height;
    fb.resize(width, height);
    editorOutput.resize(width, height);
    sceneGBuffer.resize(width, height);
    objectPickPass.resize(width, height);

    outlinePass.resize(width, height);

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
        fb,
        sceneObjectManager.getSceneObject(selectedObjectID),
        editorOutput.getFBO()
    );
}

void RendererScene::renderUI(float fps, float swapWaitMs, bool cameraCaptured)
{
   sceneRenderUI.renderUI(
        uiState,
        sceneObjectManager,
        selectedObjectID,
        fps,
        swapWaitMs,
        editorOutput.getTextureID(0),
        cameraCaptured,
        camera
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


void RendererScene::queueModelImport(const std::string& path)
{
    if (path.empty()) return;
    pendingModelImports.push_back(path);
    uiState.importFailed = false;
    uiState.importMessage = "Queued: " + std::filesystem::u8path(path).filename().string();
}

void RendererScene::acceptModelDrop(const std::vector<std::string>& paths, float x, float y)
{
    if (!sceneRenderUI.acceptsModelDrop(x, y))
    {
        uiState.importFailed = true;
        uiState.importMessage = "Drop models onto the scene view or the model import area.";
        return;
    }
    for (const auto& path : paths) queueModelImport(path);
}

void RendererScene::processModelImports()
{
    if (pendingModelImports.empty() || sceneRenderUI.gizmoUsing) return;
    const std::string path = pendingModelImports.front();
    pendingModelImports.pop_front();
    try
    {
        float distance = 4.0f;
        if (sceneConfig.renderMode == RenderMode::Lighting && viewportPixelWidth > 0 && viewportPixelHeight > 0)
        {
            GLint readFBO;
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneGBuffer.getFBO());
            float depth = 1.0f;
            glReadPixels(viewportPixelWidth / 2, viewportPixelHeight / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
            if (depth < 1.0f)
            {
                const glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                    static_cast<float>(viewportPixelWidth) / viewportPixelHeight, 0.1f, 100.0f);
                const glm::vec3 hit = glm::unProject(glm::vec3(viewportPixelWidth / 2, viewportPixelHeight / 2, depth),
                    camera.GetViewMatrix(), projection, glm::vec4(0, 0, viewportPixelWidth, viewportPixelHeight));
                distance = std::clamp(glm::length(hit - camera.Getposition()) * 0.4f, 0.15f, 4.0f);
            }
        }
        selectedObjectID = sceneObjectManager.importModel(path, sceneConfig.sceneSelection,
            camera.Getposition() + camera.GetFront() * distance, distance * 0.45f);
        sceneConfig.enableEdit = true;
        sceneConfig.renderMode = RenderMode::Lighting;
        renderModeIndex = 0;
        uiState.importFailed = false;
        uiState.importMessage = "Imported: " + std::filesystem::u8path(path).filename().string();
    }
    catch (const std::exception& error)
    {
        uiState.importFailed = true;
        uiState.importMessage = std::filesystem::u8path(path).filename().string() + ": " + error.what();
    }
}
