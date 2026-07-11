#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmath>
#include <iostream>
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
#include "rendering/core/ModelDrawer.h"
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
#include "rendering/assets/ibl/BrdfLUT.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/resources/environment/EnvironmentOption.h"
#include "rendering/imgui/ui_import.h"
#include "rendering/core/EnvironmentController.h"
#include "rendering/core/WindowContext.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 设置这次绘制的范围
    glViewport(0, 0, width, height);
    // glfwGetWindowUserPointer(window) 返回的是 void*，需要强制转换为 WindowContext* 才能访问其成员。
    // void*和WindowContext*本身都是存的地址，但编译器处理void*和WindowContext*的方式不同，所以需要强制转换。
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));

    if (!ctx) return;

    if (ctx->framebuffer)
    {
        ctx->framebuffer->resize(width, height);
    }
    if (ctx->pingpongFBO)
    {
        ctx->pingpongFBO->resize(width, height);
    }
    if (ctx->gBuffer)
    {
        ctx->gBuffer->resize(width, height);
    }
    if (ctx->ssao)
    {
        ctx->ssao->resize(width, height);
    }
};

// 并非回调，所以显式传引用
void processInput(GLFWwindow* window, WindowContext& ctx, float deltaTime)
{
    if (ctx.cursorLocked)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        ctx.camera->ProcessSmoothKeyboard(
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS,
            deltaTime
        );
    }

    bool gravePressed = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (gravePressed && !ctx.gravePressLastFrame)
    {
        ctx.cursorLocked = !ctx.cursorLocked;
        if (ctx.cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            ctx.camera->Resetmouse();
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    ctx.gravePressLastFrame = gravePressed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->cursorLocked || !ctx->camera) return;

    ctx->camera->ProcessMouseMovement(
        static_cast<float>(xpos), static_cast<float>(ypos));
}

int main()
{
    // 此时ctx仍是一个局部变量
    WindowContext ctx;
    int bfwidth = 800;
    int bfheight = 600;
    float lastFrame = 0.0f;
    float currentFrame = 0.0f;
    float swapWaitMs = 0.0f;


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
    // 到这里把ctx的地址交给GLFW,把地址存进它自己内部维护的，属于window的user pointer里。
    glfwSetWindowUserPointer(window, &ctx);

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

    glfwGetFramebufferSize(window, &bfwidth, &bfheight);

    Screenquad screenQuad;

    Framebuffer fb(bfwidth, bfheight);
    PingPongFramebuffer pingpongFBO(bfwidth, bfheight);
    GBuffer sceneGBuffer(bfwidth, bfheight);
    SSAO sceneSSAO(bfwidth, bfheight);
    Camera camera;

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

    Shader lightCubeShader(
        "../src/shader/pratice/scenerender/light_cube.vs",
        "../src/shader/pratice/scenerender/light_cube.fs"
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

    Shader basicForwardShader(
        "../src/shader/renderer/forward/unlit.vs",
        "../src/shader/renderer/forward/unlit.fs"
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
    sceneResources.lightCubeShader = &lightCubeShader;
    sceneResources.reflectShader = &cubemapShader;
    sceneResources.basicForwardShader = &basicForwardShader;
    sceneResources.reflectForwardShader = &cubemapShader;
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
    sceneConfig.sceneSelection = SceneSelection::LivingRoom;
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
    sceneConfig.forwardLightMode = ForwardLightMode::Light;

    SphereDrawer sphereDrawer(&sphereMesh, &sceneState, &sceneConfig);
    sphereDrawer.loadMaterials("../textures/PBR/");

    ModelDrawer livingRoomDrawer("../3D_model/living_room_interior_free.glb", &sceneConfig);
    livingRoomDrawer.setVisibleInScene(SceneSelection::LivingRoom);
    glm::mat4 livingRoomTransform(1.0f);
    livingRoomTransform = glm::translate(livingRoomTransform, glm::vec3(0.0f, 0.0f, -3.0f));
    livingRoomTransform = glm::scale(livingRoomTransform, glm::vec3(0.2f));
    livingRoomDrawer.setTransform(livingRoomTransform);

    DirectionalShadowMap shadowMap(4096, 4096);
    PointShadowMap pointShadowMap(1024, 1024, 1.0f, 50.0f);

    ctx.framebuffer = &fb;
    ctx.pingpongFBO = &pingpongFBO;
    ctx.gBuffer = &sceneGBuffer;
    ctx.ssao = &sceneSSAO;
    ctx.camera = &camera;

    PointShadowPass pointShadowPass(pointShadowMap, 
        *sceneResources.pointShadowMapShader, 
        sphereDrawer,
        livingRoomDrawer,
        sceneResources.registry,
        sceneResources.lightingHandles.depthCubeMap);
    SpotShadowMap spotShadowMap(1024, 1024, 1.0f, 50.0f);
    LightSettings lightSettings;
    DirectionalShadowPass directionalShadowPass(
        shadowMap,
        *sceneResources.shadowMapShader,
        sphereDrawer,
        livingRoomDrawer,
        sceneState,
        lightSettings,
        sceneResources.registry,
        sceneResources.lightingHandles.shadowMap);
    SpotShadowPass spotShadowPass(
        spotShadowMap,
        *sceneResources.shadowMapShader,
        sphereDrawer,
        livingRoomDrawer,
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

    sceneResources.brdfLUT = &brdfLUT;
    sceneResources.pingpongFBO = &pingpongFBO;

    EnvironmentController environmentController
    (
    sceneConfig,
    renderParams,
    lightSettings,
    sceneResources,
    irradianceShader,
    prefilterShader
    );

    int renderModeIndex = 0;
    int environmentIndex =
        getEnvironmentIndex(sceneConfig.environmentSelection);

    environmentController.load();
    environmentController.applyPreset();

    GeometryPass geometryPass(
        sceneResources,
        sceneConfig,
        camera,
        sphereDrawer,
        livingRoomDrawer,
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
        ssaoCommonPass,
        sphereDrawer,
        livingRoomDrawer);

    SceneRenderUI sceneRenderUI;

    SceneRenderUIState uiState
    {
        renderModeIndex,
        environmentIndex,
        sceneConfig,
        renderParams,
        lightSettings,

        false,

        [&environmentController]() {
            return environmentController.load();
        },

        [&environmentController]() {
            environmentController.applyPreset();
        }
    };

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

        sceneRenderUI.renderUI(uiState, FPS, swapWaitMs);

        processInput(window, ctx, deltaTime);

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
