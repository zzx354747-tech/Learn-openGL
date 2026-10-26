#include "imgui.h"
#include "ImGuizmo.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "rendering/core/WindowContext.h"
#include "rendering/core/RendererScene.h"
#include "../../tests/ScenePersistenceCheck.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Render-target dimensions follow the editor viewport, not the outer window.
    glViewport(0, 0, width, height);
}

// 并非回调，所以显式传引用
void processInput(GLFWwindow* window, WindowContext& ctx, float deltaTime)
{
    if (ctx.cursorLocked && ctx.rendererScene)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        ctx.rendererScene->getCamera().ProcessSmoothKeyboard(
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS,
            deltaTime
        );
    }

    bool gravePressed =
        glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;

    if (gravePressed && !ctx.gravePressLastFrame && !ImGui::GetIO().WantTextInput &&
        (!ctx.rendererScene || !ctx.rendererScene->sceneRenderUI.gizmoUsing))
    {
        ctx.cursorLocked = !ctx.cursorLocked;

        if (ctx.cursorLocked)
        {
            glfwSetInputMode(
                window, GLFW_CURSOR, GLFW_CURSOR_DISABLED
            );

            if (ctx.rendererScene)
                ctx.rendererScene->getCamera().Resetmouse();
        }
        else
        {
            glfwSetInputMode(
                window, GLFW_CURSOR, GLFW_CURSOR_NORMAL
            );
        }
    }

    ctx.gravePressLastFrame = gravePressed;

    bool editPressed =
        glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS;

    if (editPressed && !ctx.editKeyPressLastFrame && !ImGui::GetIO().WantCaptureKeyboard &&
        (!ctx.rendererScene || !ctx.rendererScene->sceneRenderUI.gizmoUsing))
    {
        if (ctx.rendererScene)
            ctx.rendererScene->toggleEditMode();
    }

    ctx.editKeyPressLastFrame = editPressed;


}

void model_drop_callback(GLFWwindow* window, int count, const char** paths)
{
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->rendererScene) return;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    std::vector<std::string> files;
    for (int i = 0; i < count; ++i) files.emplace_back(paths[i]);
    ctx->rendererScene->acceptModelDrop(files, static_cast<float>(x), static_cast<float>(y));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->cursorLocked || !ctx->rendererScene) return;

    ctx->rendererScene->getCamera().ProcessMouseMovement(
        static_cast<float>(xpos), static_cast<float>(ypos));
}

// Capture the application's own framebuffer for visual regression checks.
bool captureFrame(const char* path, int width, int height)
{
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream output(path, std::ios::binary);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        output.write(reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(y) * width * 3), width * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    return static_cast<bool>(output);
}

int main(int argc, char** argv)
{
    // 此时ctx仍是一个局部变量
    WindowContext ctx;
    int bfwidth = 800;
    int bfheight = 600;
    float lastFrame = 0.0f;
    float currentFrame = 0.0f;
    float swapWaitMs = 0.0f;
    const char* capturePath = argc == 3 && std::string(argv[1]) == "--capture-ui" ? argv[2] : nullptr;
    int frameNumber = 0;
    const bool persistenceCheck = argc == 2 && std::string(argv[1]) == "--check-scene-persistence";
    int exitCode = 0;


    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1440, 900, "Render Studio | Scene Editor", nullptr, nullptr);
    if (!window)    
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetWindowSizeLimits(window, 960, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    ctx.cursorLocked = false;
    // 到这里把ctx的地址交给GLFW,把地址存进它自己内部维护的，属于window的user pointer里。
    glfwSetWindowUserPointer(window, &ctx);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetDropCallback(window, model_drop_callback);

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
    // 默认暗色主题
    SceneRenderUI::configureStyle();
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    // 将ImGui绑定到GLFW
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // imgui使用GLSL 330版shader来画ui
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwGetFramebufferSize(window, &bfwidth, &bfheight);

    {

    RendererScene scene(bfwidth, bfheight);
    scene.sceneConfig.enableEdit = true;
    int renderWidth = bfwidth;
    int renderHeight = bfheight;

    ctx.rendererScene = &scene;

    if (persistenceCheck)
    {
        scene.resize(320, 240);
        exitCode = runScenePersistenceCheck(scene) ? 0 : 1;
        glfwSetWindowShouldClose(window, true);
    }
    while (!glfwWindowShouldClose(window))
    {
        // 准备开始新一帧ui渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float FPS = ImGui::GetIO().Framerate;
        deltaTime = std::min(deltaTime, 0.1f);

        scene.renderUI(FPS, swapWaitMs, ctx.cursorLocked);

        processInput(window, ctx, deltaTime);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);

        auto& editor = scene.sceneRenderUI;
        const ImVec2 pixelScale = ImGui::GetIO().DisplayFramebufferScale;
        const int viewportWidth = std::max(1, static_cast<int>(std::lround(editor.viewportWidth * pixelScale.x)));
        const int viewportHeight = std::max(1, static_cast<int>(std::lround(editor.viewportHeight * pixelScale.y)));
        if (viewportWidth != renderWidth || viewportHeight != renderHeight)
        {
            scene.resize(viewportWidth, viewportHeight);
            renderWidth = viewportWidth;
            renderHeight = viewportHeight;
        }
        if (editor.viewportClicked && scene.getEditMode() && !ctx.cursorLocked)
        {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const int x = std::clamp(static_cast<int>((mouse.x - editor.viewportX) * pixelScale.x), 0, renderWidth - 1);
            const int y = renderHeight - 1 - std::clamp(static_cast<int>((mouse.y - editor.viewportY) * pixelScale.y), 0, renderHeight - 1);
            scene.selectObjectAtPixel(x, y, renderWidth, renderHeight);
        }
        if (editor.viewportVisible && bfwidth > 0 && bfheight > 0)
            scene.render(renderWidth, renderHeight);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.045f, 0.055f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 渲染ImGui界面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (capturePath && ++frameNumber == 12)
        {
            if (!captureFrame(capturePath, bfwidth, bfheight))
                std::cerr << "Could not write UI capture: " << capturePath << '\n';
            glfwSetWindowShouldClose(window, true);
        }

        double beforeSwap = glfwGetTime();
        glfwSwapBuffers(window);
        double afterSwap = glfwGetTime();
        swapWaitMs = static_cast<float>((afterSwap - beforeSwap) * 1000.0);
        // A queued frame is presented before loading; imports keep the GL context on this thread.
        scene.processModelImports();
        glfwPollEvents();
    }
    glfwSetWindowUserPointer(window, nullptr);
}

    // 删除ImGui上下文
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return exitCode;
}
