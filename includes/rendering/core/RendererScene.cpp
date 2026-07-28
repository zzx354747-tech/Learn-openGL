// rendering/core/RendererScene.cpp
#include "rendering/core/RendererScene.h"
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
                      sceneResources.lightingHandles.spotShadowMap)
    , brdfLUT(shaderLibrary.brdf)
    , environmentController(sceneConfig,
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
