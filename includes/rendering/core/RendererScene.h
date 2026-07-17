// rendering/core/RendererScene.h
#pragma once

#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/framebuffer/PingPong_Framebuffer.h"
#include "rendering/resources/framebuffer/Gbuffer.h"
#include "rendering/resources/ssao/SSAO.h"
#include "scene/Camera.h"
#include "rendering/resources/shader/ShaderLibrary.h"
#include "rendering/assets/mesh/SphereMesh.h"
#include "rendering/assets/mesh/LightMesh.h"
#include "rendering/assets/mesh/SkyboxMesh.h"
#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/assets/mesh/WaterMesh.h"
#include "rendering/assets/mesh/AlpineVegetationSystem.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/resources/shadow/PointShadowMap.h"
#include "rendering/resources/shadow/SpotShadowMap.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/passes/shadow/PointShadowPass.h"
#include "rendering/passes/shadow/DirectionalShadowPass.h"
#include "rendering/passes/shadow/SpotShadowPass.h"
#include "rendering/core/EnvironmentController.h"
#include "rendering/passes/geometry/GeometryPass.h"
#include "rendering/passes/lighting/LightingPass.h"
#include "rendering/passes/ssao/SSAOCommonPass.h"
#include "rendering/core/SceneRender.h"
#include "rendering/assets/texture/Texture.h"
#include <memory>
#include "rendering/imgui/ui_import.h"
#include "rendering/resources/scene/ScenePresets.h"
#include "rendering/resources/environment/EnvironmentOption.h"
#include "rendering/core/SceneRender.h"

// 第一轮:仅第 0 层(无依赖对象)。
// 顺序为main栈上对象的构造顺序
struct RendererScene
{
    Screenquad screenQuad;

    Framebuffer fb;
    PingPongFramebuffer pingpongFBO;
    GBuffer sceneGBuffer;
    SSAO sceneSSAO;

    Camera camera;

    ShaderLibrary shaderLibrary;

    SphereMesh sphereMesh;
    LightMesh lightMesh;
    SkyboxMesh skyboxMesh;
    TerrainMesh terrainMesh;
    WaterMesh waterMesh;
    AlpineVegetationSystem vegetationSystem;

    DirectionalShadowMap shadowMap;
    PointShadowMap pointShadowMap;
    SpotShadowMap spotShadowMap;   

    ShadowResources shadowResources;

    LightSettings lightSettings;   

    SceneRenderConfig sceneConfig;
    SceneSelection previousSceneSelection = SceneSelection::LivingRoom;
    RenderParams renderParams;
    SceneRenderState sceneState;

    // 第二轮新增:第 1 层
    SceneRenderResources sceneResources;
    std::unique_ptr<GLTexture> sunTexture;
    std::unique_ptr<GLTexture> blueNoiseTexture;
    SphereDrawer sphereDrawer;
    ModelDrawer livingRoomDrawer;

    // 第三轮新增:第 2 层
    PointShadowPass pointShadowPass;
    DirectionalShadowPass directionalShadowPass;
    SpotShadowPass spotShadowPass;
    BrdfLUT brdfLUT;
    EnvironmentController environmentController;

    // 第四轮新增:第 3 层
    GeometryPass geometryPass;
    LightingPass lightingPass;
    SSAOCommonPass ssaoCommonPass;
    SceneRender sceneRender;

    SceneRenderUI sceneRenderUI;
    int renderModeIndex = 0;
    int environmentIndex = 0;
    SceneRenderUIState uiState;

    SceneRenderConfig cloudWeatherTransitionStart;
    CloudWeatherPreset activeCloudWeatherPreset = CloudWeatherPreset::Sunny;
    unsigned int activeCloudWeatherTransitionRequest = 0;
    float cloudWeatherTransitionElapsed = 0.0f;
    float automaticWeatherElapsed = 0.0f;
    int automaticWeatherStage = 0;
    double sunLastUpdateSeconds = 0.0;
    double cloudWeatherLastUpdateSeconds = 0.0;
    double vegetationExposureLastUpdateSeconds = 0.0;
    bool cloudWeatherTransitionActive = false;

    void resize(int width, int height);

    void render(int width, int height);

    void renderUI(float FPS, float swapWaitMs);

    Camera& getCamera();

    void updateCloudWeatherEvolution();

    RendererScene(int width, int height);
};
