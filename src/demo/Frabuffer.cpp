#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "rendering/core/WindowContext.h"
#include "rendering/core/RendererScene.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 设置这次绘制的范围
    glViewport(0, 0, width, height);
    // glfwGetWindowUserPointer(window) 返回的是 void*，需要强制转换为 WindowContext* 才能访问其成员。
    // void*和WindowContext*本身都是存的地址，但编译器处理void*和WindowContext*的方式不同，所以需要强制转换。
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));

    if (!ctx || !ctx->rendererScene) return;

    ctx->rendererScene->resize(width, height);
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

    bool gravePressed = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (gravePressed && !ctx.gravePressLastFrame)
    {
        ctx.cursorLocked = !ctx.cursorLocked;
        if (ctx.cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (ctx.rendererScene)
                ctx.rendererScene->getCamera().Resetmouse();
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
    if (!ctx || !ctx->cursorLocked || !ctx->rendererScene) return;

    ctx->rendererScene->getCamera().ProcessMouseMovement(
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
    // 默认暗色主题
    ImGui::StyleColorsDark();
    // 将ImGui绑定到GLFW
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // imgui使用GLSL 330版shader来画ui
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwGetFramebufferSize(window, &bfwidth, &bfheight);

    {

    RendererScene scene(bfwidth, bfheight);

    ctx.rendererScene = &scene;

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

        scene.renderUI(FPS, swapWaitMs);

        processInput(window, ctx, deltaTime);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);

        scene.render(bfwidth, bfheight);

        // 渲染ImGui界面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        double beforeSwap = glfwGetTime();
        glfwSwapBuffers(window);
        double afterSwap = glfwGetTime();
        swapWaitMs = static_cast<float>((afterSwap - beforeSwap) * 1000.0);
        glfwPollEvents();
    }
    glfwSetWindowUserPointer(window, nullptr);
}

    // 删除ImGui上下文
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
