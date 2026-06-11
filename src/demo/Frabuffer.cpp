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
#include "rendering/assets/Texture.h"
#include "rendering/postprocess/HDR_Framebuffer.h"
#include "rendering/postprocess/PingPong_Framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/LightSettings.h"
#include "rendering/assets/CubeMap.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/core/SceneRender.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/postprocess/DirectionalShadowMap.h"
#include "rendering/postprocess/PointShadowMap.h"
#include "rendering/postprocess/SpotShadowMap.h"
#include "rendering/passes/ShadowPass/DirectionalShadowPass.h"
#include "rendering/passes/ShadowPass/PointShadowPass.h"
#include "rendering/passes/ShadowPass/SpotShadowPass.h"
#include "rendering/passes/GeometryPass.h"
#include "rendering/passes/LightingPass.h"
#include "rendering/postprocess/Gbuffer.h"
#include "rendering/Model/Mesh.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/passes/SceneObjectPass.h"

Framebuffer* framebuffer = nullptr;
PingPongFramebuffer* pingpongFramebuffer = nullptr;
GBuffer* gBuffer = nullptr;
Camera camera;
bool cursorLocked = true; // 光标是否被锁定
bool gravePresslastFrame = false; // 上一帧是否按下了`键
float currentFrame = 0.0f;
float lastFrame = 0.0f;
float swapWaitMs = 0.0f;
unsigned int fbo;
int bfwidth, bfheight;

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

    GLTexture cubeDiffuseTexture("../textures/bricks2.jpg");
    GLTexture cubeNormalTexture("../textures/bricks2_normal.jpg");
    GLTexture cubeParallaxTexture("../textures/bricks2_disp.jpg");
    GLTexture secondCubeDiffuseTexture("../textures/toy/toy_box_diffuse.png");
    GLTexture secondCubeNormalTexture("../textures/toy/toy_box_normal.png");
    GLTexture secondCubeParallaxTexture("../textures/toy/toy_box_disp.png");
    GLTexture floorTexture("../textures/wooden_floor.png");

    CubeMap skybox(skyboxFaces);

    Screenquad screenQuad;

    Framebuffer fb(bfwidth, bfheight);
    framebuffer = &fb;
    PingPongFramebuffer pingpongFBO(bfwidth, bfheight);
    pingpongFramebuffer = &pingpongFBO;
    GBuffer sceneGBuffer(bfwidth, bfheight);
    gBuffer = &sceneGBuffer;

   Shader screenShader(
    "../src/shader/pratice/framebuffer/screen.vs",
    "../src/shader/pratice/framebuffer/screen.fs"
);

Shader basicCubeShader(
    "../src/shader/pratice/scenerender/basic_cube.vs",
    "../src/shader/pratice/scenerender/basic_cube.fs"
);

Shader basicPlaneShader(
    "../src/shader/pratice/scenerender/basic_plane.vs",
    "../src/shader/pratice/scenerender/basic_plane.fs"
);

Shader lightingCubeShader(
    "../src/shader/pratice/scenerender/lighting_cube.vs",
    "../src/shader/pratice/scenerender/lighting_cube.fs"
);

Shader lightingPlaneShader(
    "../src/shader/pratice/scenerender/lighting_plane.vs",
    "../src/shader/pratice/scenerender/lighting_plane.fs"
);

Shader lightCubeShader(
    "../src/shader/pratice/scenerender/light_cube.vs",
    "../src/shader/pratice/scenerender/light_cube.fs"
);

Shader cubemapShader(
    "../src/shader/pratice/scenerender/cubemap.vs",
    "../src/shader/pratice/scenerender/cubemap.fs"
);

Shader shadowDebugShader(
    "../src/shader/pratice/scenerender/shadowMap/shadowDebug.vs",
    "../src/shader/pratice/scenerender/shadowMap/shadowDebug.fs"
);

Shader shadowMapShader(
    "../src/shader/pratice/scenerender/shadowMap/shadowMap.vs",
    "../src/shader/pratice/scenerender/shadowMap/shadowMap.fs"
);

Shader pointShadowMapShader(
    "../src/shader/pratice/scenerender/shadowMap/pointShadow.vs",
    "../src/shader/pratice/scenerender/shadowMap/geometry.gs",
    "../src/shader/pratice/scenerender/shadowMap/pointShadow.fs"
);

Shader basicModelShader(
    "../src/shader/pratice/scenerender/basic_model.vs",
    "../src/shader/pratice/scenerender/basic_model.fs"
);

    Shader lightingModelShader(
        "../src/shader/pratice/scenerender/model.vs",
        "../src/shader/pratice/scenerender/model.fs"
    );

    Shader blurShader(
        "../src/shader/pratice/framebuffer/blur.vs",
        "../src/shader/pratice/framebuffer/blur.fs"
    );

    Shader geometryShader(
        "../src/shader/pratice/geometry/geometry_pass.vs",
        "../src/shader/pratice/geometry/geometry_pass.fs"
    );

    Shader lightingPassShader(
        "../src/shader/pratice/lightPass/light.vs",
        "../src/shader/pratice/lightPass/light.fs"
    );

    CubeMesh cubeMesh;
    PlaneMesh planeMesh;
    LightMesh lightMesh;
    SkyboxMesh skyboxMesh;
    Model rock_1k ("../3D_Model/rock_1k.glb");

    SceneRenderResources sceneResources;
    sceneResources.basicCubeShader = &basicCubeShader;
    sceneResources.basicPlaneShader = &basicPlaneShader;
    sceneResources.lightingCubeShader = &lightingCubeShader;
    sceneResources.lightingPlaneShader = &lightingPlaneShader;
    sceneResources.lightCubeShader = &lightCubeShader;
    sceneResources.reflectShader = &cubemapShader;
    sceneResources.shadowDebugShader = &shadowDebugShader;
    sceneResources.shadowMapShader = &shadowMapShader;
    sceneResources.pointShadowMapShader = &pointShadowMapShader;
    sceneResources.basicModelShader = &basicModelShader;
    sceneResources.lightingModelShader = &lightingModelShader;
    sceneResources.blurShader = &blurShader;
    sceneResources.geometryShader = &geometryShader;
    sceneResources.lightingPassShader = &lightingPassShader;
    sceneResources.cubeMesh = &cubeMesh;
    sceneResources.planeMesh = &planeMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;
    sceneResources.skybox = &skybox;
    sceneResources.pingpongFBO = &pingpongFBO;
    sceneResources.floorTexture = &floorTexture;
    sceneResources.cubeDiffuseTexture = &cubeDiffuseTexture;
    sceneResources.cubeNormalTexture = &cubeNormalTexture;
    sceneResources.cubeParallaxTexture = &cubeParallaxTexture;
    sceneResources.secondCubeDiffuseTexture = &secondCubeDiffuseTexture;
    sceneResources.secondCubeNormalTexture = &secondCubeNormalTexture;
    sceneResources.secondCubeParallaxTexture = &secondCubeParallaxTexture;
    sceneResources.model = &rock_1k;

    SceneRenderConfig sceneConfig;
    sceneConfig.enableFloor = true;
    sceneConfig.enableSkybox = true;
    sceneConfig.enablePointLight = true;
    sceneConfig.enableDirectionalLight = false;
    sceneConfig.enableFlashlight = false;
    sceneConfig.enableGammaCorrection = false;
    sceneConfig.enableBloom = false;
    sceneConfig.cubeEnableNormalMapping = true;
    sceneConfig.cubeEnableParallaxMapping = true;
    sceneConfig.cubeParallaxHeightScale = 0.03f;
    sceneConfig.modelEnableNormalMapping = true;
    sceneConfig.modelEnableParallaxMapping = false;
    sceneConfig.modelParallaxHeightScale = 0.03f;

    SceneRenderState sceneState;
    SceneDrawer sceneDrawer(&cubeMesh, &planeMesh, &sceneState, &rock_1k);

    DirectionalShadowMap shadowDebug(4096, 4096);
    DirectionalShadowMap shadowMap(4096, 4096);
    PointShadowMap pointShadowMap(1024, 1024, 1.0f, 25.0f);
    PointShadowPass pointShadowPass(pointShadowMap, *sceneResources.pointShadowMapShader, sceneDrawer);
    SpotShadowMap spotShadowMap(1024, 1024, 1.0f, 25.0f);
    LightSettings lightSettings;
    DirectionalShadowPass directionalShadowPass(
        shadowMap,
        *sceneResources.shadowMapShader,
        sceneDrawer,
        sceneState,
        lightSettings);
    SpotShadowPass spotShadowPass(
        spotShadowMap,
        *sceneResources.shadowMapShader,
        sceneDrawer,
        camera,
        sceneState,
        lightSettings);

    ShadowResources shadowResources;
    shadowResources.shadowMap = &shadowMap;
    shadowResources.pointShadowMap = &pointShadowMap;
    shadowResources.spotShadowMap = &spotShadowMap;

    int renderModeIndex = 0;

    SceneObjectPass objectPass(sceneResources,
        shadowResources,
        sceneConfig,
        sceneState,
        lightSettings,
        camera,
        sceneDrawer);

    GeometryPass geometryPass(
        sceneResources,
        sceneConfig,
        sceneState,
        camera,
        sceneDrawer,
        sceneGBuffer);

    LightingPass lightingPass(
        sceneResources,
        shadowResources,
        sceneConfig,
        sceneState,
        lightSettings,
        camera,
        sceneGBuffer);

    SceneRender sceneRender(camera, 
        objectPass, 
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
        sceneGBuffer);

    while (!glfwWindowShouldClose(window))
    {
        // 准备开始新一帧ui渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float lightTime = currentFrame * 0.35f;
        sceneState.lightPositions = glm::vec3(
            std::cos(lightTime) * 3.0f,
            1.5f + std::sin(currentFrame * 1.3f) * 0.5f,
            std::sin(lightTime) * 3.0f - 1.0f
        );

        float FPS = 1.0f / deltaTime;

        ImGui::Begin("Framebuffer Demo");
        ImGui::Text("welcome to framebuffer demo!");
        ImGui::Text("FPS: %.2f", FPS);
        ImGui::Text("Swap wait ms: %.3f", swapWaitMs);
        ImGui::Separator();
        const char* renderModeNames[] = {"Basic", "Lighting", "Reflection", "Shadow Debug"};
        if (ImGui::Combo("Render Mode", &renderModeIndex, renderModeNames, 4))
        {
            sceneConfig.renderMode = static_cast<RenderMode>(renderModeIndex);
        }
        ImGui::Checkbox("Floor", &sceneConfig.enableFloor);
        ImGui::Checkbox("Skybox", &sceneConfig.enableSkybox);
        ImGui::Checkbox("Point Light", &sceneConfig.enablePointLight);
        ImGui::Checkbox("Directional Light", &sceneConfig.enableDirectionalLight);
        ImGui::Checkbox("Flashlight", &sceneConfig.enableFlashlight);
        ImGui::Checkbox("Gamma Correction", &sceneConfig.enableGammaCorrection);
        ImGui::Checkbox("HDR", &sceneConfig.enableHDR);
        ImGui::Checkbox("Bloom", &sceneConfig.enableBloom);
        ImGui::Checkbox("Cube Normal Mapping", &sceneConfig.cubeEnableNormalMapping);
        ImGui::Checkbox("Cube Parallax Mapping", &sceneConfig.cubeEnableParallaxMapping);
        ImGui::SliderFloat("Cube Parallax Height Scale", &sceneConfig.cubeParallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::Checkbox("Model Normal Mapping", &sceneConfig.modelEnableNormalMapping);
        ImGui::Checkbox("Model Parallax Mapping", &sceneConfig.modelEnableParallaxMapping);
        ImGui::SliderFloat("Model Parallax Height Scale", &sceneConfig.modelParallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Exposure", &sceneConfig.exposure, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Bloom Strength", &sceneConfig.bloomStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bloom Threshold", &sceneConfig.bloomThreshold, 0.0f, 3.0f, "%.2f");
        ImGui::SliderInt("Cube Parallax Layers", &sceneConfig.cubeNumLayers, 1, 64);
        ImGui::SliderInt("Model Parallax Layers", &sceneConfig.modelNumLayers, 1, 64);
        ImGui::SliderInt("Number of Blur Passes", &sceneConfig.numBlurPasses, 1, 20);

        if (sceneConfig.enablePointLight)
        {
            ImGui::SeparatorText("Point Light Settings");

            ImGui::ColorEdit3("Point Light Ambient", glm::value_ptr(lightSettings.pointAmbient));
            ImGui::ColorEdit3("Point Light Diffuse", glm::value_ptr(lightSettings.pointDiffuse));
            ImGui::ColorEdit3("Point Light Specular", glm::value_ptr(lightSettings.pointSpecular));
            ImGui::DragFloat("Point Light Constant", &lightSettings.pointConstant, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Linear", &lightSettings.pointLinear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Quadratic", &lightSettings.pointQuadratic, 0.001f, 0.0f, 1.0f);
        }

        if (sceneConfig.enableDirectionalLight)
        {
            ImGui::SeparatorText("Directional Light Settings");

            ImGui::ColorEdit3("Directional Light Ambient", glm::value_ptr(lightSettings.sunAmbient));
            ImGui::ColorEdit3("Directional Light Diffuse", glm::value_ptr(lightSettings.sunDiffuse));
            ImGui::ColorEdit3("Directional Light Specular", glm::value_ptr(lightSettings.sunSpecular));
        }

        if (sceneConfig.enableFlashlight)
        {
            ImGui::SeparatorText("Flashlight Settings");

            ImGui::ColorEdit3("Flashlight Ambient", glm::value_ptr(lightSettings.flashAmbient));
            ImGui::ColorEdit3("Flashlight Diffuse", glm::value_ptr(lightSettings.flashDiffuse));
            ImGui::ColorEdit3("Flashlight Specular", glm::value_ptr(lightSettings.flashSpecular));
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
