#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "core/shader.h"
#include "scene/camera.h"
#include "rendering/assets/texture.h"
#include "rendering/postprocess/framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/LightMesh.h"
#include "rendering/assets/CubeMap.h"
#include "rendering/assets/SkyboxMesh.h"
#include "rendering/core/SceneRender.h"
#include "rendering/uniforms/LightUniformSetter.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/postprocess/DirectionalShadowMap.h"

Framebuffer* framebuffer = nullptr;
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
};

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

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

    GLTexture cubeTexture("../textures/wooden_box.png");
    GLTexture floorTexture("../textures/wooden_floor.png");
    CubeMap skybox(skyboxFaces);

    Screenquad screenQuad;

    Framebuffer fb(bfwidth, bfheight);
    framebuffer = &fb;

    Shader screenShader("../src/shader/pratice/framebuffer/screen.vs", "../src/shader/pratice/framebuffer/screen.fs");
    Shader basicCubeShader("../src/shader/pratice/scenerender/basic_cube.vs", "../src/shader/pratice/scenerender/basic_cube.fs");
    Shader basicPlaneShader("../src/shader/pratice/scenerender/basic_plane.vs", "../src/shader/pratice/scenerender/basic_plane.fs");
    Shader lightingCubeShader("../src/shader/pratice/scenerender/lighting_cube.vs", "../src/shader/pratice/scenerender/lighting_cube.fs");
    Shader lightingPlaneShader("../src/shader/pratice/scenerender/lighting_plane.vs", "../src/shader/pratice/scenerender/lighting_plane.fs");
    Shader lightCubeShader("../src/shader/pratice/scenerender/light_cube.vs", "../src/shader/pratice/scenerender/light_cube.fs");
    Shader cubemapShader("../src/shader/pratice/scenerender/cubemap.vs", "../src/shader/pratice/scenerender/cubemap.fs");
    Shader shadowDebugShader("../src/shader/pratice/scenerender/shadowMap/shadowDebug.vs", "../src/shader/pratice/scenerender/shadowMap/shadowDebug.fs");
    Shader shadowMapShader("../src/shader/pratice/scenerender/shadowMap/shadowMap.vs", "../src/shader/pratice/scenerender/shadowMap/shadowMap.fs");

    CubeMesh cubeMesh;
    PlaneMesh planeMesh;
    LightMesh lightMesh;
    SkyboxMesh skyboxMesh;

    SceneRenderResources sceneResources;
    sceneResources.basicCubeShader = &basicCubeShader;
    sceneResources.basicPlaneShader = &basicPlaneShader;
    sceneResources.lightingCubeShader = &lightingCubeShader;
    sceneResources.lightingPlaneShader = &lightingPlaneShader;
    sceneResources.lightCubeShader = &lightCubeShader;
    sceneResources.reflectShader = &cubemapShader;
    sceneResources.cubeMesh = &cubeMesh;
    sceneResources.planeMesh = &planeMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;
    sceneResources.skybox = &skybox;
    sceneResources.floorTexture = &floorTexture;
    sceneResources.shadowDebugShader = &shadowDebugShader;
    sceneResources.shadowMapShader = &shadowMapShader;  

    SceneRenderConfig sceneConfig;
    sceneConfig.enableFloor = true;
    sceneConfig.enableSkybox = true;
    sceneConfig.enablePointLight = true;
    sceneConfig.enableDirectionalLight = false;
    sceneConfig.enableFlashlight = false;

    SceneRenderState sceneState;
    SceneDrawer sceneDrawer(&cubeMesh, &planeMesh, &sceneState);

    DirectionalShadowMap shadowDebug(*sceneResources.shadowDebugShader, sceneDrawer, 2048, 2048);
    DirectionalShadowMap shadowMap(*sceneResources.shadowMapShader, sceneDrawer, 2048, 2048);

    ShadowResources shadowResources;
    shadowResources.shadowMap = &shadowMap;

    SceneRender sceneRender(sceneResources, shadowResources, sceneConfig, sceneState, camera, sceneDrawer);
    RenderMode renderMode = RenderMode::Basic;
    int renderModeIndex = 0;

    LightSettings lightSettings;

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

        ImGui::Begin("Framebuffer Demo");
        ImGui::Text("welcome to framebuffer demo!");
        ImGui::Text("FPS: %.2f", FPS);
        ImGui::Text("Swap wait ms: %.3f", swapWaitMs);
        ImGui::Separator();
        const char* renderModeNames[] = {"Basic", "Lighting", "Reflection", "Shadow Debug"};
        if (ImGui::Combo("Render Mode", &renderModeIndex, renderModeNames, 4))
        {
            renderMode = static_cast<RenderMode>(renderModeIndex);
        }
        ImGui::Checkbox("Floor", &sceneConfig.enableFloor);
        ImGui::Checkbox("Skybox", &sceneConfig.enableSkybox);
        ImGui::Checkbox("Point Light", &sceneConfig.enablePointLight);
        ImGui::Checkbox("Directional Light", &sceneConfig.enableDirectionalLight);
        ImGui::Checkbox("Flashlight", &sceneConfig.enableFlashlight);

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

        sceneRender.setRenderMode(renderMode);
        sceneRender.setconfig(sceneConfig);
        sceneRender.setLightData(lightSettings);

        processInput(window, deltaTime);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);

        sceneRender.render(
            bfwidth, 
            bfheight, 
            cubeTexture, 
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
