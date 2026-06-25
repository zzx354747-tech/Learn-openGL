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
#include "rendering/assets/Texture.h"
#include "rendering/postprocess/HDR_Framebuffer.h"
#include "rendering/postprocess/PingPong_Framebuffer.h"
#include "rendering/postprocess/Screenquad.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/SphereMesh.h"
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
#include "rendering/passes/SSAOCommonPass.h"
#include "rendering/postprocess/Gbuffer.h"
#include "rendering/postprocess/SSAO.h"
#include "rendering/Model/Mesh.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderResources.h"
#include "rendering/passes/SceneObjectPass.h"
#include "rendering/assets/HDRTexture.h"
#include "rendering/assets/EnvCubemap.h"
#include "rendering/assets/IrradianceMap.h"
#include "rendering/assets/PrefilterMap.h"
#include "rendering/assets/BrdfLUT.h"

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
        HDRLoadOptions{true, 100.0f, 2.0f}
    },
    {
        "Sunny",
        "../textures/skybox/sunny.hdr",
        EnvironmentSelection::Sunny,
        HDRLoadOptions{true, 100.0f, 2.0f}
    },
    {
        "Night N8 3K",
        "../textures/skybox/Night_08_3K.hdr",
        EnvironmentSelection::NightN8_3K,
        HDRLoadOptions{true, 100.0f, 2.0f}
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
    GLTexture floorTexture("../textures/PBR/Ground104_2K-PNG/Ground104_2K-PNG_Color.png");
    GLTexture clearSphereAlbedoTexture(255, 255, 255, 0);
    GLTexture defaultMetallicTexture(0, 0, 0);
    GLTexture defaultRoughnessTexture(255, 255, 255);
    GLTexture groundNormalTexture("../textures/PBR/Ground104_2K-PNG/Ground104_2K-PNG_NormalGL.png");
    GLTexture groundRoughnessTexture("../textures/PBR/Ground104_2K-PNG/Ground104_2K-PNG_Roughness.png");
    GLTexture groundDisplacementTexture("../textures/PBR/Ground104_2K-PNG/Ground104_2K-PNG_Displacement.png");

    GLTexture bricks066AlbedoTexture("../textures/PBR/Bricks066_2K-PNG/Bricks066_2K-PNG_Color.png");
    GLTexture bricks066NormalTexture("../textures/PBR/Bricks066_2K-PNG/Bricks066_2K-PNG_NormalGL.png");
    GLTexture bricks066RoughnessTexture("../textures/PBR/Bricks066_2K-PNG/Bricks066_2K-PNG_Roughness.png");
    GLTexture bricks066DisplacementTexture("../textures/PBR/Bricks066_2K-PNG/Bricks066_2K-PNG_Displacement.png");

    GLTexture grass005AlbedoTexture("../textures/PBR/Grass005_2K-PNG/Grass005_2K-PNG_Color.png");
    GLTexture grass005NormalTexture("../textures/PBR/Grass005_2K-PNG/Grass005_2K-PNG_NormalGL.png");
    GLTexture grass005RoughnessTexture("../textures/PBR/Grass005_2K-PNG/Grass005_2K-PNG_Roughness.png");
    GLTexture grass005DisplacementTexture("../textures/PBR/Grass005_2K-PNG/Grass005_2K-PNG_Displacement.png");

    GLTexture gravel023AlbedoTexture("../textures/PBR/Gravel023_2K-PNG/Gravel023_2K-PNG_Color.png");
    GLTexture gravel023NormalTexture("../textures/PBR/Gravel023_2K-PNG/Gravel023_2K-PNG_NormalGL.png");
    GLTexture gravel023RoughnessTexture("../textures/PBR/Gravel023_2K-PNG/Gravel023_2K-PNG_Roughness.png");
    GLTexture gravel023DisplacementTexture("../textures/PBR/Gravel023_2K-PNG/Gravel023_2K-PNG_Displacement.png");

    GLTexture marble012AlbedoTexture("../textures/PBR/Marble012_2K-PNG/Marble012_2K-PNG_Color.png");
    GLTexture marble012NormalTexture("../textures/PBR/Marble012_2K-PNG/Marble012_2K-PNG_NormalGL.png");
    GLTexture marble012RoughnessTexture("../textures/PBR/Marble012_2K-PNG/Marble012_2K-PNG_Roughness.png");
    GLTexture marble012DisplacementTexture("../textures/PBR/Marble012_2K-PNG/Marble012_2K-PNG_Displacement.png");

    GLTexture metal003AlbedoTexture("../textures/PBR/Metal003_2K-PNG/Metal003_2K-PNG_Color.png");
    GLTexture metal003NormalTexture("../textures/PBR/Metal003_2K-PNG/Metal003_2K-PNG_NormalGL.png");
    GLTexture metal003RoughnessTexture("../textures/PBR/Metal003_2K-PNG/Metal003_2K-PNG_Roughness.png");
    GLTexture metal003MetallicTexture("../textures/PBR/Metal003_2K-PNG/Metal003_2K-PNG_Metalness.png");
    GLTexture metal003DisplacementTexture("../textures/PBR/Metal003_2K-PNG/Metal003_2K-PNG_Displacement.png");

    GLTexture metal034AlbedoTexture("../textures/PBR/Metal034_2K-PNG/Metal034_2K-PNG_Color.png");
    GLTexture metal034NormalTexture("../textures/PBR/Metal034_2K-PNG/Metal034_2K-PNG_NormalGL.png");
    GLTexture metal034RoughnessTexture("../textures/PBR/Metal034_2K-PNG/Metal034_2K-PNG_Roughness.png");
    GLTexture metal034MetallicTexture("../textures/PBR/Metal034_2K-PNG/Metal034_2K-PNG_Metalness.png");
    GLTexture metal034DisplacementTexture("../textures/PBR/Metal034_2K-PNG/Metal034_2K-PNG_Displacement.png");

    GLTexture metal055AlbedoTexture("../textures/PBR/Metal055A_2K-PNG/Metal055A_2K-PNG_Color.png");
    GLTexture metal055NormalTexture("../textures/PBR/Metal055A_2K-PNG/Metal055A_2K-PNG_NormalGL.png");
    GLTexture metal055RoughnessTexture("../textures/PBR/Metal055A_2K-PNG/Metal055A_2K-PNG_Roughness.png");
    GLTexture metal055MetallicTexture("../textures/PBR/Metal055A_2K-PNG/Metal055A_2K-PNG_Metalness.png");
    GLTexture metal055DisplacementTexture("../textures/PBR/Metal055A_2K-PNG/Metal055A_2K-PNG_Displacement.png");

    GLTexture rock060AlbedoTexture("../textures/PBR/Rock060_2K-PNG/Rock060_2K-PNG_Color.png");
    GLTexture rock060NormalTexture("../textures/PBR/Rock060_2K-PNG/Rock060_2K-PNG_NormalGL.png");
    GLTexture rock060RoughnessTexture("../textures/PBR/Rock060_2K-PNG/Rock060_2K-PNG_Roughness.png");
    GLTexture rock060DisplacementTexture("../textures/PBR/Rock060_2K-PNG/Rock060_2K-PNG_Displacement.png");

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

    Shader envCubemapShader(
        "../src/shader/pratice/skybox/envCubemap.vs",
        "../src/shader/pratice/skybox/envCubemap.fs"
    );

    Shader irradianceShader(
        "../src/shader/pratice/skybox/envCubemap.vs",
        "../src/shader/pratice/skybox/irradiance.fs"
    );

    Shader prefilterShader(
        "../src/shader/pratice/skybox/envCubemap.vs",
        "../src/shader/pratice/skybox/prefilter.fs"
    );

    Shader brdfShader(
        "../src/shader/pratice/skybox/brdf.vs",
        "../src/shader/pratice/skybox/brdf.fs"
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

    Shader ssaoShader(
        "../src/shader/pratice/SSAO/ssao_common.vs",
        "../src/shader/pratice/SSAO/ssao.fs"
    );

    Shader ssaoBlurShader(
        "../src/shader/pratice/SSAO/ssao_common.vs",
        "../src/shader/pratice/SSAO/ssao_blur.fs"
    );

    CubeMesh cubeMesh;
    PlaneMesh planeMesh;
    SphereMesh sphereMesh;
    LightMesh lightMesh;
    SkyboxMesh skyboxMesh;
    Model livingRoomModel("../3D_model/living_room_interior_free.glb");
    Model modernCityModel("../3D_model/modern_city_block.glb");

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
    sceneResources.ssaoShader = &ssaoShader;
    sceneResources.ssaoBlurShader = &ssaoBlurShader;
    sceneResources.envCubemapShader = &envCubemapShader;
    sceneResources.cubeMesh = &cubeMesh;
    sceneResources.planeMesh = &planeMesh;
    sceneResources.sphereMesh = &sphereMesh;
    sceneResources.lightMesh = &lightMesh;
    sceneResources.skyboxMesh = &skyboxMesh;
    sceneResources.floorTexture = &floorTexture;
    sceneResources.cubeDiffuseTexture = &cubeDiffuseTexture;
    sceneResources.cubeNormalTexture = &cubeNormalTexture;
    sceneResources.cubeParallaxTexture = &cubeParallaxTexture;
    sceneResources.secondCubeDiffuseTexture = &secondCubeDiffuseTexture;
    sceneResources.secondCubeNormalTexture = &secondCubeNormalTexture;
    sceneResources.secondCubeParallaxTexture = &secondCubeParallaxTexture;
    sceneResources.clearSphereAlbedoTexture = &clearSphereAlbedoTexture;
    sceneResources.defaultRoughnessTexture = &defaultRoughnessTexture;
    sceneResources.defaultMetallicTexture = &defaultMetallicTexture;
    sceneResources.floorPBRMaterial = {
        &floorTexture,
        &groundNormalTexture,
        &groundRoughnessTexture,
        &defaultMetallicTexture,
        &groundDisplacementTexture
    };
    PBRMaterialTextures groundFloorPBRMaterial = sceneResources.floorPBRMaterial;
    sceneResources.materialSpherePBRMaterials[0] = {
        &bricks066AlbedoTexture,
        &bricks066NormalTexture,
        &bricks066RoughnessTexture,
        &defaultMetallicTexture,
        &bricks066DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[1] = {
        &grass005AlbedoTexture,
        &grass005NormalTexture,
        &grass005RoughnessTexture,
        &defaultMetallicTexture,
        &grass005DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[2] = {
        &gravel023AlbedoTexture,
        &gravel023NormalTexture,
        &gravel023RoughnessTexture,
        &defaultMetallicTexture,
        &gravel023DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[3] = {
        &marble012AlbedoTexture,
        &marble012NormalTexture,
        &marble012RoughnessTexture,
        &defaultMetallicTexture,
        &marble012DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[4] = {
        &metal003AlbedoTexture,
        &metal003NormalTexture,
        &metal003RoughnessTexture,
        &metal003MetallicTexture,
        &metal003DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[5] = {
        &metal034AlbedoTexture,
        &metal034NormalTexture,
        &metal034RoughnessTexture,
        &metal034MetallicTexture,
        &metal034DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[6] = {
        &metal055AlbedoTexture,
        &metal055NormalTexture,
        &metal055RoughnessTexture,
        &metal055MetallicTexture,
        &metal055DisplacementTexture
    };
    sceneResources.materialSpherePBRMaterials[7] = {
        &rock060AlbedoTexture,
        &rock060NormalTexture,
        &rock060RoughnessTexture,
        &defaultMetallicTexture,
        &rock060DisplacementTexture
    };
    sceneResources.model = &livingRoomModel;

    SceneRenderConfig sceneConfig;
    sceneConfig.enableFloor = true;
    sceneConfig.enableSkybox = true;
    sceneConfig.enablePointLight = true;
    sceneConfig.enableDirectionalLight = false;
    sceneConfig.enableFlashlight = false;
    sceneConfig.enableGammaCorrection = false;
    sceneConfig.enableBloom = false;
    sceneConfig.enableSSAO = true;
    sceneConfig.enablePBR = true;
    sceneConfig.enableIBL = true;
    sceneConfig.enableClearSphere = true;
    sceneConfig.fixedAmbientColor = glm::vec3(0.08f);
    sceneConfig.fixedAmbientStrength = 1.0f;
    sceneConfig.iblAmbientTint = glm::vec3(1.0f);
    sceneConfig.iblAmbientStrength = 1.0f;
    sceneConfig.cubeEnableNormalMapping = true;
    sceneConfig.cubeEnableParallaxMapping = true;
    sceneConfig.cubeParallaxHeightScale = 0.03f;
    sceneConfig.modelEnableNormalMapping = true;
    sceneConfig.modelEnableParallaxMapping = false;
    sceneConfig.modelParallaxHeightScale = 0.03f;

    SceneRenderState sceneState;
    sceneConfig.renderMode = RenderMode::Lighting;

    SceneDrawer sceneDrawer(&cubeMesh,
        &planeMesh,
        &sphereMesh,
        &sceneState,
        &sceneConfig,
        &livingRoomModel,
        &modernCityModel);

    DirectionalShadowMap shadowDebug(4096, 4096);
    DirectionalShadowMap shadowMap(4096, 4096);
    PointShadowMap pointShadowMap(1024, 1024, 1.0f, 50.0f);
    PointShadowPass pointShadowPass(pointShadowMap, *sceneResources.pointShadowMapShader, sceneDrawer);
    SpotShadowMap spotShadowMap(1024, 1024, 1.0f, 50.0f);
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

    BrdfLUT brdfLUT(brdfShader);
    std::unique_ptr<HDRTexture> hdrTexture;
    std::unique_ptr<EnvCubemap> skybox;
    std::unique_ptr<IrradianceMap> irradianceMap;
    std::unique_ptr<PrefilterMap> prefilterMap;

    sceneResources.brdfLUT = &brdfLUT;
    sceneResources.pingpongFBO = &pingpongFBO;

    auto applyExtractedSun = [&](const ExtractedLight& extractedSun)
    {
        if (extractedSun.valid)
        {
            lightSettings.sunDirection = -extractedSun.direction;
            lightSettings.sunDiffuse = extractedSun.color;
            lightSettings.sunSpecular = extractedSun.color;
            lightSettings.sunAmbient = glm::vec3(0.0f);
            lightSettings.sunIntensity = 1.0f;
            lightSettings.sunIntensityScale = 1.0f;
            lightSettings.sunExtractedFromEnvironment = true;
            sceneConfig.enableDirectionalLight = true;
        }
        else
        {
            lightSettings.sunDiffuse = glm::vec3(0.0f);
            lightSettings.sunSpecular = glm::vec3(0.0f);
            lightSettings.sunAmbient = glm::vec3(0.0f);
            lightSettings.sunExtractedFromEnvironment = false;
            sceneConfig.enableDirectionalLight = false;
        }
    };

    auto loadEnvironment = [&]()
    {
        int environmentIndex = getEnvironmentIndex(sceneConfig.environmentSelection);

        hdrTexture = std::make_unique<HDRTexture>();
        hdrTexture->load(kEnvironmentOptions[environmentIndex].path, kEnvironmentOptions[environmentIndex].loadOptions);
        skybox = std::make_unique<EnvCubemap>(*hdrTexture, *sceneResources.envCubemapShader);
        irradianceMap = std::make_unique<IrradianceMap>(*skybox, irradianceShader);
        prefilterMap = std::make_unique<PrefilterMap>(*skybox, prefilterShader);

        sceneResources.skybox = skybox.get();
        sceneResources.irradianceMap = irradianceMap.get();
        sceneResources.prefilterMap = prefilterMap.get();
        applyExtractedSun(hdrTexture->getExtractedSun());
    };
    loadEnvironment();

    int renderModeIndex = 1;
    int sceneIndex = 0;
    int environmentIndex = getEnvironmentIndex(sceneConfig.environmentSelection);
    int floorMaterialIndex = 0;
    const char* floorMaterialNames[] = {
        "Ground",
        "Bricks066",
        "Grass005",
        "Gravel023",
        "Marble012",
        "Metal003",
        "Metal034",
        "Metal055A",
        "Rock060"
    };
    auto applyFloorMaterial = [&]()
    {
        if (floorMaterialIndex <= 0)
        {
            sceneResources.floorPBRMaterial = groundFloorPBRMaterial;
            return;
        }

        const int materialSphereIndex = floorMaterialIndex - 1;
        if (materialSphereIndex < static_cast<int>(MaterialSphereCount))
        {
            sceneResources.floorPBRMaterial =
                sceneResources.materialSpherePBRMaterials[materialSphereIndex];
        }
    };
    auto enableFullMaterialMapping = [&]()
    {
        sceneConfig.cubeEnableNormalMapping = true;
        sceneConfig.cubeEnableParallaxMapping = true;
        sceneConfig.floorEnableNormalMapping = true;
        sceneConfig.floorEnableParallaxMapping = true;
        sceneConfig.modelEnableNormalMapping = true;
        sceneConfig.modelEnableParallaxMapping = true;
    };
    auto applyEnvironmentPreset = [&]()
    {
        sceneConfig.renderMode = RenderMode::Lighting;
        renderModeIndex = 1;
        sceneConfig.enableFloor = true;
        sceneConfig.enableSkybox = true;
        sceneConfig.enableModel = true;
        sceneConfig.enableGammaCorrection = true;
        sceneConfig.enableHDR = true;
        sceneConfig.enableBloom = true;
        sceneConfig.enableSSAO = true;
        sceneConfig.enablePBR = true;
        sceneConfig.enableIBL = true;
        enableFullMaterialMapping();
        sceneConfig.cubeNumLayers = 32;
        sceneConfig.floorNumLayers = 32;
        sceneConfig.modelNumLayers = 32;

        switch (sceneConfig.environmentSelection)
        {
        case EnvironmentSelection::Sunny:
            floorMaterialIndex = 1; // Bricks066
            sceneConfig.enablePointLight = false;
            sceneConfig.enableDirectionalLight = true;
            sceneConfig.enableFlashlight = false;
            sceneConfig.enableClearSphere = true;
            sceneConfig.fixedAmbientStrength = 0.1f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f);
            sceneConfig.iblAmbientStrength = 1.4f;
            sceneConfig.ssaoStrength = 1.5f;
            sceneConfig.exposure = 0.9f;
            sceneConfig.bloomStrength = 0.6f;
            sceneConfig.bloomThreshold = 1.3f;
            sceneConfig.cubeParallaxHeightScale = 0.009f;
            sceneConfig.floorParallaxHeightScale = 0.009f;
            sceneConfig.modelParallaxHeightScale = 0.009f;
            lightSettings.sunDiffuse = glm::vec3(38.0f, 31.0f, 15.0f);
            lightSettings.sunSpecular = lightSettings.sunDiffuse;
            lightSettings.sunAmbient = glm::vec3(1.0f, 1.5f, 2.5f);
            lightSettings.sunIntensity = 1.3f;
            lightSettings.sunIntensityScale = 0.52f;
            lightSettings.sunShadowStrength = 0.94f;
            sceneConfig.directionalShadowLightSize = 0.004f;
            sceneConfig.directionalShadowBlockerSearchRadius = 0.006f;
            sceneConfig.directionalShadowMinFilterRadius = 0.001f;
            sceneConfig.directionalShadowMaxFilterRadius = 0.005f;
            break;

        case EnvironmentSelection::NightN8_3K:
            floorMaterialIndex = 3; // Gravel023
            sceneConfig.enablePointLight = false;
            sceneConfig.enableDirectionalLight = true;
            sceneConfig.enableFlashlight = false;
            sceneConfig.enableClearSphere = true;
            sceneConfig.fixedAmbientStrength = 0.0f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f, 0.6f, 0.9f);
            sceneConfig.iblAmbientStrength = 2.8f;
            sceneConfig.ssaoStrength = 1.8f;
            sceneConfig.exposure = 0.8f;
            sceneConfig.bloomStrength = 2.2f;
            sceneConfig.bloomThreshold = 0.9f;
            sceneConfig.cubeParallaxHeightScale = 0.009f;
            sceneConfig.floorParallaxHeightScale = 0.009f;
            sceneConfig.modelParallaxHeightScale = 0.009f;
            sceneConfig.cubeNumLayers = 48;
            sceneConfig.floorNumLayers = 48;
            sceneConfig.modelNumLayers = 48;
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
            floorMaterialIndex = 4; // Marble012
            sceneConfig.enablePointLight = true;
            sceneConfig.enableDirectionalLight = false;
            sceneConfig.enableFlashlight = false;
            sceneConfig.enableClearSphere = true;
            sceneConfig.fixedAmbientStrength = 0.05f;
            sceneConfig.iblAmbientTint = glm::vec3(1.0f);
            sceneConfig.iblAmbientStrength = 0.15f;
            sceneConfig.ssaoStrength = 2.5f;
            sceneConfig.exposure = 1.4f;
            sceneConfig.bloomStrength = 1.8f;
            sceneConfig.bloomThreshold = 0.7f;
            sceneConfig.cubeParallaxHeightScale = 0.009f;
            sceneConfig.floorParallaxHeightScale = 0.009f;
            sceneConfig.modelParallaxHeightScale = 0.009f;
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

        applyFloorMaterial();
    };
    applyEnvironmentPreset();

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

    SSAOCommonPass ssaoCommonPass(
        sceneResources,
        sceneSSAO,
        screenQuad,
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
        sceneGBuffer,
        ssaoCommonPass,
        sceneSSAO);

    while (!glfwWindowShouldClose(window))
    {
        // 准备开始新一帧ui渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::vec3 sceneCenter = sceneDrawer.getActiveSceneWorldCenter();
        glm::vec3 sceneSize = sceneDrawer.getActiveSceneWorldSize();
        sceneState.lightPositions = glm::vec3(
            sceneCenter.x - sceneSize.x * 0.12f,
            -0.5f + sceneSize.y * 0.68f,
            sceneCenter.z + sceneSize.z * 0.08f
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
        const char* environmentNames[] = {"Night", "Sunny", "Night N8 3K"};
        if (ImGui::Combo("Environment", &environmentIndex, environmentNames, 3))
        {
            sceneConfig.environmentSelection = kEnvironmentOptions[environmentIndex].selection;
            loadEnvironment();
            applyEnvironmentPreset();
        }
        const char* sceneNames[] = {"Default", "Modern City"};
        if (ImGui::Combo("Scene", &sceneIndex, sceneNames, 2))
        {
            sceneConfig.sceneSelection = static_cast<SceneSelection>(sceneIndex);
        }
        ImGui::Checkbox("Floor", &sceneConfig.enableFloor);
        if (ImGui::Combo(
                "Floor Material",
                &floorMaterialIndex,
                floorMaterialNames,
                static_cast<int>(MaterialSphereCount) + 1))
        {
            applyFloorMaterial();
        }
        ImGui::Checkbox("Skybox", &sceneConfig.enableSkybox);
        ImGui::Checkbox("Model", &sceneConfig.enableModel);
        ImGui::Checkbox("Point Light", &sceneConfig.enablePointLight);
        ImGui::Checkbox("Directional Light", &sceneConfig.enableDirectionalLight);
        ImGui::Checkbox("Flashlight", &sceneConfig.enableFlashlight);
        ImGui::Checkbox("Gamma Correction", &sceneConfig.enableGammaCorrection);
        ImGui::Checkbox("HDR", &sceneConfig.enableHDR);
        ImGui::Checkbox("Bloom", &sceneConfig.enableBloom);
        ImGui::Checkbox("SSAO", &sceneConfig.enableSSAO);
        ImGui::Checkbox("PBR", &sceneConfig.enablePBR);
        ImGui::Checkbox("IBL", &sceneConfig.enableIBL);
        ImGui::Checkbox("Clear Sphere", &sceneConfig.enableClearSphere);
        ImGui::ColorEdit3("Fixed Ambient Color", glm::value_ptr(sceneConfig.fixedAmbientColor));
        ImGui::DragFloat("Fixed Ambient Strength", &sceneConfig.fixedAmbientStrength, 0.01f, 0.0f, 4.0f);
        ImGui::ColorEdit3("IBL Ambient Tint", glm::value_ptr(sceneConfig.iblAmbientTint));
        ImGui::DragFloat("IBL Ambient Strength", &sceneConfig.iblAmbientStrength, 0.01f, 0.0f, 4.0f);
        ImGui::SliderFloat("SSAO Strength", &sceneConfig.ssaoStrength, 0.0f, 4.0f, "%.2f");
        ImGui::Checkbox("Cube Normal Mapping", &sceneConfig.cubeEnableNormalMapping);
        ImGui::Checkbox("Cube Parallax Mapping", &sceneConfig.cubeEnableParallaxMapping);
        ImGui::SliderFloat("Cube Parallax Height Scale", &sceneConfig.cubeParallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::Checkbox("Floor Normal Mapping", &sceneConfig.floorEnableNormalMapping);
        ImGui::Checkbox("Floor Parallax Mapping", &sceneConfig.floorEnableParallaxMapping);
        ImGui::SliderFloat("Floor Parallax Height Scale", &sceneConfig.floorParallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Floor Bump Normal Strength", &sceneConfig.floorBumpNormalStrength, 0.0f, 10.0f, "%.2f");
        ImGui::Checkbox("Model Normal Mapping", &sceneConfig.modelEnableNormalMapping);
        ImGui::Checkbox("Model Parallax Mapping", &sceneConfig.modelEnableParallaxMapping);
        ImGui::SliderFloat("Model Parallax Height Scale", &sceneConfig.modelParallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Model Bump Normal Strength", &sceneConfig.modelBumpNormalStrength, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("Exposure", &sceneConfig.exposure, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Bloom Strength", &sceneConfig.bloomStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bloom Threshold", &sceneConfig.bloomThreshold, 0.0f, 3.0f, "%.2f");
        ImGui::SliderInt("Cube Parallax Layers", &sceneConfig.cubeNumLayers, 1, 64);
        ImGui::SliderInt("Floor Parallax Layers", &sceneConfig.floorNumLayers, 1, 64);
        ImGui::SliderInt("Model Parallax Layers", &sceneConfig.modelNumLayers, 1, 64);
        ImGui::SliderInt("Number of Blur Passes", &sceneConfig.numBlurPasses, 1, 20);

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
