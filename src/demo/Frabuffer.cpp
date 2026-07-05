#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/texture/Texture.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/framebuffer/PingPong_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/assets/mesh/CubeMesh.h"
#include "rendering/assets/mesh/PlaneMesh.h"
#include "rendering/assets/mesh/SphereMesh.h"
#include "rendering/assets/mesh/LightMesh.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/assets/texture/CubeMap.h"
#include "rendering/assets/mesh/SkyboxMesh.h"
#include "rendering/core/SceneRender.h"
#include "rendering/resources/shadow/DirectionalShadowMap.h"
#include "rendering/resources/shadow/PointShadowMap.h"
#include "rendering/resources/shadow/SpotShadowMap.h"
#include "rendering/passes/shadow/DirectionalShadowPass.h"
#include "rendering/passes/shadow/PointShadowPass.h"
#include "rendering/passes/shadow/SpotShadowPass.h"
#include "rendering/passes/geometry/GeometryPass.h"
#include "rendering/passes/lighting/LightingPass.h"
#include "rendering/passes/ssao/SSAOCommonPass.h"
#include "rendering/resources/framebuffer/Gbuffer.h"
#include "rendering/resources/ssao/SSAO.h"
#include "rendering/modelload/Mesh.h"
#include "rendering/modelload/Model.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/assets/texture/HDRTexture.h"
#include "rendering/assets/texture/EnvCubemap.h"
#include "rendering/assets/ibl/IrradianceMap.h"
#include "rendering/assets/ibl/PrefilterMap.h"
#include "rendering/assets/ibl/BrdfLUT.h"
#include "rendering/uniforms/RenderParams.h"

Framebuffer* framebuffer = nullptr;
PingPongFramebuffer* pingpongFramebuffer = nullptr;
GBuffer* gBuffer = nullptr;
SSAO* ssao = nullptr;
Camera camera;
bool cursorLocked = true; // 光标是否被锁定
bool gravePresslastFrame = false; // 上一帧是否按下了`键
float currentFrame = 0.0f;
float lastFrame = 0.0f;
float swapWaitMs = 0.0f;
unsigned int fbo;
int bfwidth, bfheight;

struct EnvironmentOption
{
    const char* name;
    const char* path;
    EnvironmentSelection selection;
    HDRLoadOptions loadOptions;
};

static const EnvironmentOption kEnvironmentOptions[] =
{
    {
        "Night",
        "../textures/skybox/night.hdr",
        EnvironmentSelection::Night,
        HDRLoadOptions{true, 100.0f}
    },
    {
        "Sunny",
        "../textures/skybox/sunny.hdr",
        EnvironmentSelection::Sunny,
        HDRLoadOptions{true, 100.0f}
    },
    {
        "Night N8 3K",
        "../textures/skybox/Night_08_3K.hdr",
        EnvironmentSelection::NightN8_3K,
        HDRLoadOptions{true, 100.0f}
    },
};

static int getEnvironmentIndex(EnvironmentSelection selection)
{
    for (int i = 0; i < static_cast<int>(sizeof(kEnvironmentOptions) / sizeof(kEnvironmentOptions[0])); ++i)
    {
        if (kEnvironmentOptions[i].selection == selection)
        {
            return i;
        }
    }

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 设置这次绘制的范围
    glViewport(0, 0, width, height);
    if (framebuffer)
    {
        framebuffer->resize(width, height);
    }
    if (pingpongFramebuffer)
    {
        pingpongFramebuffer->resize(width, height);
    }
    if (gBuffer)
    {
        gBuffer->resize(width, height);
    }
    if (ssao)
    {
        ssao->resize(width, height);
    }
};

void processInput(GLFWwindow* window, float deltaTime)
{
    if (cursorLocked)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        camera.ProcessSmoothKeyboard(
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS,
            deltaTime
        );
    }   

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !gravePresslastFrame)
    {
        cursorLocked = !cursorLocked;
        if (cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera.Resetmouse();
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    gravePresslastFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cursorLocked) return;
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}
    
int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Framebuffer Demo", nullptr, nullptr);
    if (!window)    
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // 启用垂直同步
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    // 创建ImGui上下文
    ImGui::CreateContext();
    // 拿到ImGui的IO对象
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // 防止编译器警告
    (void)io;
    // 默认暗色主题
    ImGui::StyleColorsDark();
    // 将ImGui绑定到GLFW
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // imgui使用GLSL 330版shader来画ui
    ImGui_ImplOpenGL3_Init("#version 330 core");

    std::vector<std::string> skyboxFaces
    {
        "../textures/skybox/right.jpg",
        "../textures/skybox/left.jpg",
        "../textures/skybox/top.jpg",
        "../textures/skybox/bottom.jpg",
        "../textures/skybox/front.jpg",
        "../textures/skybox/back.jpg"
    };

    glfwGetFramebufferSize(window, &bfwidth, &bfheight);

    Screenquad screenQuad;

    Framebuffer fb(bfwidth, bfheight);
    framebuffer = &fb;
    PingPongFramebuffer pingpongFBO(bfwidth, bfheight);
    pingpongFramebuffer = &pingpongFBO;
    GBuffer sceneGBuffer(bfwidth, bfheight);
    gBuffer = &sceneGBuffer;
    SSAO sceneSSAO(bfwidth, bfheight);
    ssao = &sceneSSAO;

    Shader screenShader(
    "../src/shader/renderer/postprocess/screen.vs",
    "../src/shader/renderer/postprocess/screen.fs"
    );

    Shader cubemapShader(
    "../src/shader/renderer/forward/reflection.vs",
    "../src/shader/renderer/forward/reflection.fs"
    );

    Shader shadowDebugShader(
    "../src/shader/renderer/shadow/debug.vs",
    "../src/shader/renderer/shadow/debug.fs"
    );

    Shader shadowMapShader(
    "../src/shader/renderer/shadow/directional.vs",
    "../src/shader/renderer/shadow/directional.fs"
    );

    Shader pointShadowMapShader(
    "../src/shader/renderer/shadow/point.vs",
    "../src/shader/renderer/shadow/point.gs",
    "../src/shader/renderer/shadow/point.fs"
    );


    Shader envCubemapShader(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/env_cubemap.fs"
    );

    Shader irradianceShader(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/irradiance.fs"
    );

    Shader prefilterShader(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/prefilter.fs"
    );

    Shader brdfShader(
        "../src/shader/renderer/ibl/brdf.vs",
        "../src/shader/renderer/ibl/brdf.fs"
    );

    Shader blurShader(
        "../src/shader/renderer/postprocess/blur.vs",
        "../src/shader/renderer/postprocess/blur.fs"
    );

    Shader geometryPBRShader(
        "../src/shader/renderer/deferred/geometry.vs",
        "../src/shader/renderer/deferred/geometry.fs"
    );

    Shader lightingPassShader(
        "../src/shader/renderer/deferred/lighting.vs",
        "../src/shader/renderer/deferred/lighting.fs"
    );

    Shader ssaoShader(
        "../src/shader/renderer/ssao/common.vs",
        "../src/shader/renderer/ssao/ssao.fs"
    );

    Shader ssaoBlurShader(
        "../src/shader/renderer/ssao/common.vs",
        "../src/shader/renderer/ssao/blur.fs"
    );

    SphereMesh sphereMesh;
    LightMesh lightMesh;
    SkyboxMesh skyboxMesh;

    SceneRenderResources sceneResources;
    sceneResources.declareLightingPassResources();
    sceneResources.reflectShader = &cubemapShader;
    sceneResources.shadowDebugShader = &shadowDebugShader;
    sceneResources.shadowMapShader = &shadowMapShader;
    sceneResources.pointShadowMapShader = &pointShadowMapShader;
    sceneResources.blurShader = &blurShader;
    sceneResources.geometryPBRShader = &geometryPBRShader;
    sceneResources.lightingPassShader = &lightingPassShader;
    sceneResources.ssaoShader = &ssaoShader;
    sceneResources.ssaoBlurShader = &ssaoBlurShader;
    sceneResources.envCubemapShader = &envCubemapShader;
    sceneResources.sphereMesh = &sphereMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;

    SceneRenderConfig sceneConfig;
    sceneConfig.enableSkybox = true;
    sceneConfig.enablePointLight = true;
    sceneConfig.enableDirectionalLight = false;
    sceneConfig.enableFlashlight = false;
    sceneConfig.enableGammaCorrection = false;
    sceneConfig.enableBloom = false;
    sceneConfig.enableSSAO = true;
    sceneConfig.enablePBR = true;
    sceneConfig.enableIBL = true;
    sceneConfig.fixedAmbientColor = glm::vec3(0.08f);
    sceneConfig.fixedAmbientStrength = 1.0f;
    sceneConfig.iblAmbientTint = glm::vec3(1.0f);
    sceneConfig.iblAmbientStrength = 1.0f;
    sceneConfig.phongDiffuseStrength = 0.55f;
    sceneConfig.phongSpecularStrength = 0.18f;
    sceneConfig.phongIBLDiffuseStrength = 1.25f;
    sceneConfig.phongIBLSpecularStrength = 0.35f;
    RenderParams renderParams;

    SceneRenderState sceneState;
    sceneConfig.renderMode = RenderMode::Lighting;

    SphereDrawer sphereDrawer(&sphereMesh, &sceneState, &sceneConfig);
    sphereDrawer.loadMaterials("../textures/PBR/");
    DirectionalShadowMap shadowDebug(4096, 4096);
    DirectionalShadowMap shadowMap(4096, 4096);
    PointShadowMap pointShadowMap(1024, 1024, 1.0f, 50.0f);
    PointShadowPass pointShadowPass(pointShadowMap, 
        *sceneResources.pointShadowMapShader, 
        sphereDrawer,
        sceneResources.registry,
        sceneResources.lightingHandles.depthCubeMap);
    SpotShadowMap spotShadowMap(1024, 1024, 1.0f, 50.0f);
    LightSettings lightSettings;
    DirectionalShadowPass directionalShadowPass(
        shadowMap,
        *sceneResources.shadowMapShader,
        sphereDrawer,
        sceneState,
        lightSettings,
        sceneResources.registry,
        sceneResources.lightingHandles.shadowMap);
    SpotShadowPass spotShadowPass(
        spotShadowMap,
        *sceneResources.shadowMapShader,
        sphereDrawer,
        camera,
        sceneState,
        lightSettings,
        sceneResources.registry,
        sceneResources.lightingHandles.spotShadowMap);

    ShadowResources shadowResources;
    shadowResources.shadowMap = &shadowMap;
    shadowResources.pointShadowMap = &pointShadowMap;
    shadowResources.spotShadowMap = &spotShadowMap;

    // 资源类注册表做法
    BrdfLUT brdfLUT(brdfShader);
    sceneResources.registry.setTexture(
    sceneResources.lightingHandles.brdfLUT,
    brdfLUT.GetID()
    );

    std::unique_ptr<HDRTexture> hdrTexture;
    std::unique_ptr<EnvCubemap> skybox;
    std::unique_ptr<IrradianceMap> irradianceMap;
    std::unique_ptr<PrefilterMap> prefilterMap;

    sceneResources.brdfLUT = &brdfLUT;
    sceneResources.pingpongFBO = &pingpongFBO;

    auto applyExtractedSun = [&](const ExtractedLight& extractedSun)
    {
        const glm::vec3 sunSourceDirection = extractedSun.valid
            ? extractedSun.direction
            : extractedSun.brightestDirection;

        lightSettings.sunDirection = -sunSourceDirection;
        lightSettings.sunExtractedFromEnvironment = extractedSun.valid;
    };

    auto loadEnvironment = [&]()
    {
        int environmentIndex = getEnvironmentIndex(sceneConfig.environmentSelection);
        HDRLoadOptions loadOptions = kEnvironmentOptions[environmentIndex].loadOptions;
        loadOptions.sunThreshold = sceneConfig.sunThreshold;

        auto nextHdrTexture = std::make_unique<HDRTexture>();
        if (!nextHdrTexture->load(kEnvironmentOptions[environmentIndex].path, loadOptions))
        {
            std::cerr << "Failed to switch environment: "
                      << kEnvironmentOptions[environmentIndex].name << std::endl;
            return;
        }

        auto nextSkybox = std::make_unique<EnvCubemap>(
            *nextHdrTexture,
            *sceneResources.envCubemapShader);
        auto nextIrradianceMap = std::make_unique<IrradianceMap>(
            *nextSkybox,
            irradianceShader);
        auto nextPrefilterMap = std::make_unique<PrefilterMap>(
            *nextSkybox,
            prefilterShader);

        if (!nextSkybox->isReady() ||
            !nextIrradianceMap->isReady() ||
            !nextPrefilterMap->isReady())
        {
            std::cerr << "Failed to build environment cubemaps: "
                      << kEnvironmentOptions[environmentIndex].name << std::endl;
            return;
        }

        hdrTexture = std::move(nextHdrTexture);
        skybox = std::move(nextSkybox);
        irradianceMap = std::move(nextIrradianceMap);
        prefilterMap = std::move(nextPrefilterMap);

        sceneResources.skybox = skybox.get();
        sceneResources.irradianceMap = irradianceMap.get();
        sceneResources.prefilterMap = prefilterMap.get();
        sceneResources.registry.setTexture(
            sceneResources.lightingHandles.irradianceMap,
            irradianceMap->GetID()
        );
        sceneResources.registry.setTexture(
            sceneResources.lightingHandles.prefilterMap,
            prefilterMap->GetID()
        );
        applyExtractedSun(hdrTexture->getExtractedSun());
    };
    loadEnvironment();

    int renderModeIndex = 0;
    int environmentIndex = getEnvironmentIndex(sceneConfig.environmentSelection);
    auto applyEnvironmentPreset = [&]()
    {
        sceneConfig.renderMode = RenderMode::Lighting;
        renderModeIndex = 0;
        sceneConfig.enableSkybox = true;
        sceneConfig.enableGammaCorrection = true;
        sceneConfig.enableHDR = true;
        sceneConfig.enableBloom = true;
        sceneConfig.enableSSAO = true;
        sceneConfig.enablePBR = true;
        sceneConfig.enableIBL = true;
        sceneConfig.phongDiffuseStrength = 0.55f;
        sceneConfig.phongSpecularStrength = 0.18f;
        sceneConfig.phongIBLDiffuseStrength = 1.25f;
        sceneConfig.phongIBLSpecularStrength = 0.35f;
        renderParams.enableNormalMapping = true;
        renderParams.enableParallaxMapping = false;
        renderParams.bumpNormalStrength = 1.0f;
        renderParams.numLayers = 32;
        renderParams.parallaxHeightScale = 0.1f;

        switch (sceneConfig.environmentSelection)
        {
        case EnvironmentSelection::Sunny:
            sceneConfig.enablePointLight = false;
            sceneConfig.enableDirectionalLight = true;
            sceneConfig.enableFlashlight = false;
            sceneConfig.fixedAmbientStrength = 0.1f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f);
            sceneConfig.iblAmbientStrength = 1.4f;
            sceneConfig.phongDiffuseStrength = 0.42f;
            sceneConfig.phongSpecularStrength = 0.10f;
            sceneConfig.phongIBLDiffuseStrength = 1.15f;
            sceneConfig.phongIBLSpecularStrength = 0.26f;
            sceneConfig.ssaoStrength = 1.5f;
            sceneConfig.exposure = 0.9f;
            sceneConfig.bloomStrength = 0.6f;
            sceneConfig.bloomThreshold = 1.3f;
            lightSettings.sunDiffuse = glm::vec3(38.0f, 31.0f, 15.0f);
            lightSettings.sunSpecular = lightSettings.sunDiffuse;
            lightSettings.sunAmbient = glm::vec3(1.0f, 1.5f, 2.5f);
            lightSettings.sunIntensity = 0.7f;
            lightSettings.sunIntensityScale = 0.52f;
            lightSettings.sunShadowStrength = 0.94f;
            sceneConfig.directionalShadowLightSize = 0.004f;
            sceneConfig.directionalShadowBlockerSearchRadius = 0.006f;
            sceneConfig.directionalShadowMinFilterRadius = 0.001f;
            sceneConfig.directionalShadowMaxFilterRadius = 0.005f;
            break;

        case EnvironmentSelection::NightN8_3K:
            sceneConfig.enablePointLight = false;
            sceneConfig.enableDirectionalLight = true;
            sceneConfig.enableFlashlight = false;
            sceneConfig.fixedAmbientStrength = 0.0f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f, 0.6f, 0.9f);
            sceneConfig.iblAmbientStrength = 2.8f;
            sceneConfig.phongDiffuseStrength = 0.48f;
            sceneConfig.phongSpecularStrength = 0.12f;
            sceneConfig.phongIBLDiffuseStrength = 0.95f;
            sceneConfig.phongIBLSpecularStrength = 0.24f;
            sceneConfig.ssaoStrength = 1.8f;
            sceneConfig.exposure = 0.8f;
            sceneConfig.bloomStrength = 2.2f;
            sceneConfig.bloomThreshold = 0.9f;
            renderParams.numLayers = 48;
            lightSettings.sunDiffuse = glm::vec3(2.0f, 14.0f, 25.0f);
            lightSettings.sunSpecular = lightSettings.sunDiffuse;
            lightSettings.sunAmbient = glm::vec3(0.5f, 0.0f, 0.8f);
            lightSettings.sunIntensity = 0.45f;
            lightSettings.sunIntensityScale = 0.5f;
            lightSettings.sunShadowStrength = 0.81f;
            sceneConfig.directionalShadowLightSize = 0.015f;
            sceneConfig.directionalShadowBlockerSearchRadius = 0.015f;
            sceneConfig.directionalShadowMinFilterRadius = 0.002f;
            sceneConfig.directionalShadowMaxFilterRadius = 0.012f;
            break;

        case EnvironmentSelection::Night:
        default:
            sceneConfig.enablePointLight = true;
            sceneConfig.enableDirectionalLight = false;
            sceneConfig.enableFlashlight = false;
            sceneConfig.fixedAmbientStrength = 0.05f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f);
            sceneConfig.iblAmbientStrength = 0.15f;
            sceneConfig.phongDiffuseStrength = 0.55f;
            sceneConfig.phongSpecularStrength = 0.16f;
            sceneConfig.phongIBLDiffuseStrength = 1.65f;
            sceneConfig.phongIBLSpecularStrength = 0.42f;
            sceneConfig.ssaoStrength = 2.5f;
            sceneConfig.exposure = 1.4f;
            sceneConfig.bloomStrength = 1.8f;
            sceneConfig.bloomThreshold = 0.7f;
            lightSettings.pointDiffuse = glm::vec3(1.0f, 0.5f, 0.1f);
            lightSettings.pointSpecular = glm::vec3(1.0f, 0.6f, 0.25f);
            lightSettings.pointIntensity = 15.0f;
            lightSettings.pointAmbientIntensity = 0.2f;
            lightSettings.pointShadowStrength = 0.98f;
            lightSettings.pointConstant = 1.0f;
            lightSettings.pointLinear = 0.09f;
            lightSettings.pointQuadratic = 0.032f;
            lightSettings.flashDiffuse = glm::vec3(0.8f, 0.9f, 1.0f);
            lightSettings.flashSpecular = glm::vec3(1.0f);
            lightSettings.flashIntensity = 12.0f;
            lightSettings.flashShadowStrength = 0.9f;
            lightSettings.flashConstant = 1.0f;
            lightSettings.flashLinear = 0.045f;
            lightSettings.flashQuadratic = 0.0075f;
            lightSettings.flashCutOff = 0.96f;
            lightSettings.flashOuterCutOff = 0.91f;
            break;
        }

    };
    applyEnvironmentPreset();

    GeometryPass geometryPass(
        sceneResources,
        sceneConfig,
        sceneState,
        camera,
        sphereDrawer,
        sceneGBuffer,
        renderParams);

    LightingPass lightingPass(
        sceneResources,
        shadowResources,
        sceneConfig,
        sceneState,
        lightSettings,
        camera);

    SSAOCommonPass ssaoCommonPass(
        sceneResources,
        sceneSSAO,
        screenQuad,
        camera,
        sceneGBuffer);

    SceneRender sceneRender(camera, 
        shadowResources, 
        sceneResources, 
        sceneConfig, 
        sceneState, 
        lightSettings, 
        directionalShadowPass,
        pointShadowPass, 
        spotShadowPass,
        geometryPass,
        lightingPass,
        sceneGBuffer,
        ssaoCommonPass);

    while (!glfwWindowShouldClose(window))
    {
        // 准备开始新一帧ui渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float FPS = 1.0f / deltaTime;

        ImGui::Begin("Deferred PBR Renderer");
        ImGui::Text("FPS: %.2f", FPS);
        ImGui::Text("Swap wait ms: %.3f", swapWaitMs);

        ImGui::SeparatorText("Render");
        const char* renderModeNames[] = {"Lighting", "Shadow Debug"};
        if (ImGui::Combo("Render Mode", &renderModeIndex, renderModeNames, 2))
        {
            sceneConfig.renderMode = (renderModeIndex == 1)
                ? RenderMode::ShadowDebug
                : RenderMode::Lighting;
        }
        const char* environmentNames[] = {"Night", "Sunny", "Night N8 3K"};
        if (ImGui::Combo("Environment", &environmentIndex, environmentNames, 3))
        {
            sceneConfig.environmentSelection = kEnvironmentOptions[environmentIndex].selection;
            loadEnvironment();
            applyEnvironmentPreset();
        }
        ImGui::Checkbox("Skybox", &sceneConfig.enableSkybox);
        ImGui::Checkbox("Gamma Correction", &sceneConfig.enableGammaCorrection);
        ImGui::Checkbox("HDR", &sceneConfig.enableHDR);
        ImGui::Checkbox("PBR", &sceneConfig.enablePBR);
        ImGui::Checkbox("IBL", &sceneConfig.enableIBL);

        ImGui::SeparatorText("Ambient");
        ImGui::ColorEdit3("Fixed Ambient Color", glm::value_ptr(sceneConfig.fixedAmbientColor));
        ImGui::DragFloat("Fixed Ambient Strength", &sceneConfig.fixedAmbientStrength, 0.01f, 0.0f, 4.0f);
        ImGui::ColorEdit3("IBL Ambient Tint", glm::value_ptr(sceneConfig.iblAmbientTint));
        ImGui::DragFloat("IBL Ambient Strength", &sceneConfig.iblAmbientStrength, 0.01f, 0.0f, 4.0f);

        ImGui::SeparatorText("Phong Settings");
        ImGui::DragFloat("Phong Diffuse Strength", &sceneConfig.phongDiffuseStrength, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Phong Specular Strength", &sceneConfig.phongSpecularStrength, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Phong IBL Diffuse", &sceneConfig.phongIBLDiffuseStrength, 0.01f, 0.0f, 6.0f);
        ImGui::DragFloat("Phong IBL Specular", &sceneConfig.phongIBLSpecularStrength, 0.01f, 0.0f, 4.0f);

        ImGui::SeparatorText("Material Mapping");
        ImGui::Checkbox("Normal Mapping", &renderParams.enableNormalMapping);
        ImGui::Checkbox("Parallax Mapping", &renderParams.enableParallaxMapping);
        ImGui::SliderFloat("Parallax Height Scale", &renderParams.parallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Bump Normal Strength", &renderParams.bumpNormalStrength, 0.0f, 10.0f, "%.2f");
        ImGui::SliderInt("Parallax Layers", &renderParams.numLayers, 1, 64);

        ImGui::SeparatorText("Post Process");
        ImGui::Checkbox("SSAO", &sceneConfig.enableSSAO);
        ImGui::SliderFloat("SSAO Strength", &sceneConfig.ssaoStrength, 0.0f, 4.0f, "%.2f");
        ImGui::Checkbox("Bloom", &sceneConfig.enableBloom);
        ImGui::SliderFloat("Exposure", &sceneConfig.exposure, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Bloom Strength", &sceneConfig.bloomStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bloom Threshold", &sceneConfig.bloomThreshold, 0.0f, 3.0f, "%.2f");
        ImGui::SliderInt("Number of Blur Passes", &sceneConfig.numBlurPasses, 1, 20);

        ImGui::SeparatorText("Lights");
        ImGui::Checkbox("Point Light", &sceneConfig.enablePointLight);
        ImGui::Checkbox("Directional Light", &sceneConfig.enableDirectionalLight);
        ImGui::Checkbox("Flashlight", &sceneConfig.enableFlashlight);
        ImGui::DragFloat("Sun Threshold", &sceneConfig.sunThreshold, 1.0f, 0.0f, 1000.0f, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            loadEnvironment();
        }

        if (sceneConfig.enablePointLight)
        {
            ImGui::SeparatorText("Point Light Settings");

            ImGui::ColorEdit3("Point Light Ambient", glm::value_ptr(lightSettings.pointAmbient));
            ImGui::ColorEdit3("Point Light Diffuse", glm::value_ptr(lightSettings.pointDiffuse));
            ImGui::ColorEdit3("Point Light Specular", glm::value_ptr(lightSettings.pointSpecular));
            ImGui::DragFloat("Light Brightness", &lightSettings.pointIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::DragFloat("Ambient Brightness", &lightSettings.pointAmbientIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Point Shadow Strength", &lightSettings.pointShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Point Light Constant", &lightSettings.pointConstant, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Linear", &lightSettings.pointLinear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Quadratic", &lightSettings.pointQuadratic, 0.001f, 0.0f, 1.0f);
        }

        if (sceneConfig.enableDirectionalLight)
        {
            ImGui::SeparatorText("Directional Light Settings");

            constexpr ImGuiColorEditFlags lightColorFlags =
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
            if (ImGui::ColorEdit3(
                    "Directional Light Color",
                    glm::value_ptr(lightSettings.sunDiffuse),
                    lightColorFlags))
            {
                lightSettings.sunSpecular = lightSettings.sunDiffuse;
            }
            ImGui::ColorEdit3(
                "Directional Light Ambient",
                glm::value_ptr(lightSettings.sunAmbient),
                lightColorFlags);
            ImGui::DragFloat("Directional Light Intensity", &lightSettings.sunIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Sun Intensity", &lightSettings.sunIntensityScale, 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Directional Shadow Strength", &lightSettings.sunShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("PCSS Light Size", &sceneConfig.directionalShadowLightSize, 0.0f, 0.03f, "%.4f");
            ImGui::SliderFloat("PCSS Blocker Search", &sceneConfig.directionalShadowBlockerSearchRadius, 0.0f, 0.03f, "%.4f");
            ImGui::SliderFloat("PCSS Min Filter", &sceneConfig.directionalShadowMinFilterRadius, 0.0f, 0.01f, "%.4f");
            ImGui::SliderFloat("PCSS Max Filter", &sceneConfig.directionalShadowMaxFilterRadius, 0.0f, 0.04f, "%.4f");
            ImGui::Text("Sun from HDR: %s", lightSettings.sunExtractedFromEnvironment ? "yes" : "no");
        }

        if (sceneConfig.enableFlashlight)
        {
            ImGui::SeparatorText("Flashlight Settings");

            ImGui::ColorEdit3("Flashlight Ambient", glm::value_ptr(lightSettings.flashAmbient));
            ImGui::ColorEdit3("Flashlight Diffuse", glm::value_ptr(lightSettings.flashDiffuse));
            ImGui::ColorEdit3("Flashlight Specular", glm::value_ptr(lightSettings.flashSpecular));
            ImGui::DragFloat("Flashlight Intensity", &lightSettings.flashIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Flashlight Shadow Strength", &lightSettings.flashShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Flashlight Constant", &lightSettings.flashConstant, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight Linear", &lightSettings.flashLinear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight Quadratic", &lightSettings.flashQuadratic, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight CutOff", &lightSettings.flashCutOff, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight OuterCutOff", &lightSettings.flashOuterCutOff, 0.001f, 0.0f, 1.0f);
        }

        ImGui::End();

        processInput(window, deltaTime);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);

        sceneRender.render(
            bfwidth, 
            bfheight, 
            screenShader, 
            screenQuad, 
            fb
        );

        // 渲染ImGui界面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        double beforeSwap = glfwGetTime();
        glfwSwapBuffers(window);
        double afterSwap = glfwGetTime();
        swapWaitMs = static_cast<float>((afterSwap - beforeSwap) * 1000.0);
        glfwPollEvents();
}
    // 删除ImGui上下文
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
