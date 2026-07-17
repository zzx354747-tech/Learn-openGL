#include "imgui.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <array>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <string>
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

int main(int argc, char** argv)
{
#ifdef OPENGL_PROJECT_ROOT
    // All renderer assets intentionally stay as loose project files. Anchor the
    // legacy ../src, ../textures and ../3D_model paths regardless of how the
    // executable was launched (IDE, Explorer, Codex, or the helper batch file).
    std::error_code workingDirectoryError;
    std::filesystem::current_path(
        std::filesystem::path(OPENGL_PROJECT_ROOT) / "build",
        workingDirectoryError);
    if (workingDirectoryError)
    {
        std::cerr << "Failed to set renderer working directory: "
                  << workingDirectoryError.message() << std::endl;
    }
#endif

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
    const bool shaderCheck = argc > 1 && std::string(argv[1]) == "--shader-check";
    const bool waterCheck = argc > 1 && std::string(argv[1]) == "--water-check";
    const bool waterEffectsCheck = argc > 1 &&
        std::string(argv[1]) == "--water-effects-check";
    const bool fujiPreview = argc > 1 &&
        std::string(argv[1]) == "--fuji-preview";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    if (shaderCheck || waterCheck || waterEffectsCheck)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

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

    if (shaderCheck || waterCheck)
    {
        ShaderLibrary shaderCheckLibrary;
        if (waterCheck)
        {
            TerrainMesh terrainCheck;
            WaterMesh waterCheckMesh(terrainCheck);
            if (terrainCheck.getLakeRegions().size() != 7u ||
                terrainCheck.getLakeDataTexture() == 0u)
            {
                std::cerr << "Water check failed: expected seven uploaded lake regions"
                          << std::endl;
                return 2;
            }
            const auto& lakes = terrainCheck.getLakeRegions();
            const auto& terrainSettings = terrainCheck.getSettings();
            const std::array<glm::vec2, 2> lakeCenters{{
                terrainSettings.lakeCenter, terrainSettings.meadowLakeCenter}};
            for (std::size_t i = 0; i < lakeCenters.size(); ++i)
            {
                const float floorHeight = terrainCheck.sampleHeight(
                    lakeCenters[i].x, lakeCenters[i].y);
                const float authoredDepth = terrainCheck.sampleWaterDepth(
                    lakeCenters[i].x, lakeCenters[i].y);
                if (authoredDepth < lakes[i].maximumDepth * 0.90f ||
                    std::abs((lakes[i].waterLevel - floorHeight) - authoredDepth) > 2.0f)
                {
                    std::cerr << "Water check failed: lake floor/LDM coupling mismatch"
                              << std::endl;
                    return 3;
                }
            }

            SceneRenderConfig coverageConfig;
            const float lowerBlendWidth = coverageConfig.terrainRockStart -
                                          coverageConfig.terrainGrassEnd;
            const float upperBlendWidth = coverageConfig.terrainSnowEnd -
                                          coverageConfig.terrainSnowStart;
            const float pureRockWidth = coverageConfig.terrainSnowStart -
                                        coverageConfig.terrainRockStart;
            std::cout << "Terrain blend widths: grass/rock=" << lowerBlendWidth
                      << ", rock/snow=" << upperBlendWidth
                      << ", pure rock=" << pureRockWidth << std::endl;
            if (lowerBlendWidth > 0.035f || upperBlendWidth > 0.035f ||
                pureRockWidth < 0.10f)
            {
                std::cerr << "Water check failed: terrain transition bands are too wide"
                          << std::endl;
                return 4;
            }
            if (!coverageConfig.water.enableCaustics ||
                coverageConfig.water.causticStrength < 2.0f ||
                coverageConfig.water.causticDepthEnd <
                    terrainCheck.getMaximumWaterDepth())
            {
                std::cerr << "Water check failed: default caustics do not cover the lake floor"
                          << std::endl;
                return 5;
            }
            double mountainSamples = 0.0;
            double weightedSnowSamples = 0.0;
            constexpr int CoverageGrid = 128;
            for (int z = 0; z < CoverageGrid; ++z)
            {
                for (int x = 0; x < CoverageGrid; ++x)
                {
                    const float worldX = -terrainSettings.size * 0.5f +
                        terrainSettings.size * (x + 0.5f) / CoverageGrid;
                    const float worldZ = -terrainSettings.size * 0.5f +
                        terrainSettings.size * (z + 0.5f) / CoverageGrid;
                    const float normalizedHeight = glm::clamp(
                        (terrainCheck.sampleHeight(worldX, worldZ) -
                         terrainSettings.baseHeight) / terrainSettings.mountainHeight,
                        0.0f, 1.0f);
                    if (normalizedHeight < coverageConfig.terrainGrassEnd)
                        continue;
                    const float heightSnow = glm::smoothstep(
                        coverageConfig.terrainSnowStart,
                        coverageConfig.terrainSnowEnd, normalizedHeight);
                    weightedSnowSamples += heightSnow;
                    mountainSamples += 1.0;
                }
            }
            const double snowCoverage = mountainSamples > 0.0
                ? weightedSnowSamples / mountainSamples : 0.0;
            std::cout << "Snow coverage of mountain samples: "
                      << snowCoverage * 100.0 << "%" << std::endl;
            if (snowCoverage < 0.30)
            {
                std::cerr << "Water check failed: snow coverage is below 30%"
                          << std::endl;
                return 6;
            }
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

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

    int waterEffectsResult = 0;
    {

    RendererScene scene(bfwidth, bfheight);

    if (waterEffectsCheck || fujiPreview)
    {
        scene.sceneConfig.sceneSelection = SceneSelection::FujiTerrain;
        scene.sceneConfig.renderMode = RenderMode::Lighting;
        scene.sceneConfig.enableWater = true;
        scene.sceneConfig.enableDirectionalLight = true;
        scene.sceneConfig.enableTimeOfDay = false;
        scene.sceneConfig.enableAutomaticWeather = false;
        scene.sceneConfig.enableVolumetricClouds = false;
        scene.sceneConfig.enableGodRays = false;
        scene.sceneConfig.daylightFactor = 1.0f;
    }

    ctx.rendererScene = &scene;

    int renderedFrameCount = 0;

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

        if (!fujiPreview)
            scene.renderUI(FPS, swapWaitMs);

        processInput(window, ctx, deltaTime);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);

        scene.render(bfwidth, bfheight);

        // 渲染ImGui界面
        ImGui::Render();
        if (!fujiPreview)
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        double beforeSwap = glfwGetTime();
        glfwSwapBuffers(window);
        double afterSwap = glfwGetTime();
        swapWaitMs = static_cast<float>((afterSwap - beforeSwap) * 1000.0);
        glfwPollEvents();
        ++renderedFrameCount;
        if (waterEffectsCheck && renderedFrameCount >= 3)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (waterEffectsCheck)
    {
        const LightingPass::CausticMapStats stats =
            scene.lightingPass.inspectCausticMap();
        const WaterRenderSettings& water = scene.sceneConfig.water;
        const float normalizedIorSpread = std::abs(water.iorRGB.b -
                                                   water.iorRGB.r) / 0.006f;
        const float representativeDispersionPixels =
            water.dispersionMaxPixels * water.dispersionStrength *
            normalizedIorSpread *
            std::exp(-6.0f * water.dispersionDepthFalloff);
        std::cout << "Caustic atlas: dynamicMax="
                  << stats.maximumDynamicDensity
                  << ", referenceMax=" << stats.maximumReferenceDensity
                  << ", focusedExcessMax=" << stats.maximumFocusedExcess
                  << std::endl;
        std::cout << "Representative dispersion separation: "
                  << representativeDispersionPixels << " px" << std::endl;
        if (!stats.valid || stats.maximumDynamicDensity < 0.01f ||
            stats.maximumReferenceDensity < 0.01f ||
            stats.maximumFocusedExcess < 0.02f ||
            representativeDispersionPixels < 0.75f)
        {
            std::cerr << "Water effects check failed: caustic or dispersion "
                         "energy is not visible" << std::endl;
            waterEffectsResult = 7;
        }
    }
    glfwSetWindowUserPointer(window, nullptr);
}

    // 删除ImGui上下文
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return waterEffectsResult;
}
