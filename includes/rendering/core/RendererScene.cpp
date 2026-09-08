// rendering/core/RendererScene.cpp
#include "rendering/core/RendererScene.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

RendererScene::RendererScene(int width, int height)
    : fb(width, height)
    , pingpongFBO(width, height)
    , sceneGBuffer(width, height)
    , sceneSSAO(width, height)
    , shadowMap(4096, 4096)
    , pointShadowMap(1024, 1024, 1.0f, 50.0f)
    , spotShadowMap(1024, 1024, 1.0f, 50.0f)
    , sceneConfig(makeLivingRoomPreset())
    , sphereDrawer(&sphereMesh, &sceneState, &sceneConfig)
    , livingRoomDrawer("../3D_model/living_room_interior_free.glb", &sceneConfig)
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
                    renderParams)
    , lightingPass(sceneResources,
                    shadowResources,
                    sceneConfig,
                    sceneState,
                    lightSettings,
                    camera)

    , downSamplePassL1(std::max(1, width / 2),
                    std::max(1, height / 2),
                    shaderLibrary.downSample,
                    screenQuad)
    , downSamplePassL2(std::max(1, width / 4),
                    std::max(1, height / 4),
                    shaderLibrary.downSample,
                    screenQuad)
    , downSamplePassL3(std::max(1, width / 8),
                    std::max(1, height / 8),
                    shaderLibrary.downSample,
                    screenQuad)
    , downSamplePassL4(std::max(1, width / 16),
                    std::max(1, height / 16),
                    shaderLibrary.downSample,
                    screenQuad)
    , downSamplePassL5(std::max(1, width / 32),
                    std::max(1, height / 32),
                    shaderLibrary.downSample,
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
                    downSamplePassL1,
                    downSamplePassL2,
                    downSamplePassL3,
                    downSamplePassL4,
                    downSamplePassL5,
                    pointShadowPass,
                    spotShadowPass,
                    geometryPass,
                    lightingPass,
                    sceneGBuffer,
                    ssaoCommonPass,
                    sphereDrawer,
                    livingRoomDrawer)

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
    sceneResources.pingpongFBO = &pingpongFBO;
    sceneResources.downSamplePassL1 = &downSamplePassL1;
    sceneResources.downSamplePassL2 = &downSamplePassL2;
    sceneResources.downSamplePassL3 = &downSamplePassL3;
    sceneResources.downSamplePassL4 = &downSamplePassL4;
    sceneResources.downSamplePassL5 = &downSamplePassL5;

    environmentController.load();
    environmentController.applyPreset();
}

void RendererScene::resize(int width, int height)
{
    fb.resize(width, height);
    pingpongFBO.resize(width, height);
    sceneGBuffer.resize(width, height);
    sceneSSAO.resize(width, height);
    brightPrefilterPass.resize(width, height);
    downSamplePassL1.resize(
        std::max(1, width / 2),
        std::max(1, height / 2));
    downSamplePassL2.resize(
        std::max(1, width / 4),
        std::max(1, height / 4));
    downSamplePassL3.resize(
        std::max(1, width / 8),
        std::max(1, height / 8));
    downSamplePassL4.resize(
        std::max(1, width / 16),
        std::max(1, height / 16));
    downSamplePassL5.resize(
        std::max(1, width / 32),
        std::max(1, height / 32));
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
    sceneRenderUI.renderUI(uiState, fps, swapWaitMs);
}

Camera& RendererScene::getCamera()
{
    return camera;
}
