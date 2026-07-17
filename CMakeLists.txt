cmake_minimum_required(VERSION 3.10)
project(OpenGLPractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(assimp CONFIG QUIET)
find_package(ZLIB QUIET)

if(TARGET assimp::assimp)
    set(ASSIMP_LINK_TARGET assimp::assimp)
else()
    set(ASSIMP_LINK_TARGET assimp)
endif()

if(TARGET ZLIB::ZLIB)
    set(ZLIB_LINK_TARGET ZLIB::ZLIB)
else()
    set(ZLIB_LINK_TARGET z)
endif()

set(ASSIMP_INSTALL_DIR ${CMAKE_SOURCE_DIR}/third_party/assimp/install)
if(EXISTS ${ASSIMP_INSTALL_DIR}/include AND EXISTS ${ASSIMP_INSTALL_DIR}/lib)
    set(ASSIMP_INCLUDE_DIR ${ASSIMP_INSTALL_DIR}/include)
    set(ASSIMP_GENERATED_INCLUDE_DIR ${ASSIMP_INSTALL_DIR}/include)
    set(ASSIMP_LIBRARY_DIR ${ASSIMP_INSTALL_DIR}/lib)
else()
    set(ASSIMP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/assimp/include)
    set(ASSIMP_GENERATED_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/assimp/build/include)
    set(ASSIMP_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/third_party/assimp/build/lib)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_SOURCE_DIR}/third_party/stb
    ${ASSIMP_INCLUDE_DIR}
    ${ASSIMP_GENERATED_INCLUDE_DIR}
)

link_directories(${ASSIMP_LIBRARY_DIR})

# ── 平台系统库 ────────────────────────────────────────────
if(APPLE)
    set(PLATFORM_LIBS
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    set(PLATFORM_LIBS
        opengl32
        gdi32
        user32
        shell32
    )
else()  # Linux
    find_package(OpenGL REQUIRED)
    set(PLATFORM_LIBS OpenGL::GL dl m)
endif()
# ─────────────────────────────────────────────────────────

set(GLAD_SOURCES
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/gl.c
)

# ── ImGui ─────────────────────────────────────────────────
set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master
    ${CMAKE_SOURCE_DIR}/third_party/imgui-master/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ── 辅助宏：统一设置输出目录 ──────────────────────────────
macro(set_output_dir target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    )
endmacro()

# ── 各 Target ─────────────────────────────────────────────

add_executable(3Ddemo
    src/demo/3Ddemo.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(3Ddemo glfw ${PLATFORM_LIBS})
set_output_dir(3Ddemo)

add_executable(3Dbkp
    src/example/3Dbkp.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(3Dbkp glfw ${PLATFORM_LIBS})
set_output_dir(3Dbkp)

add_executable(rot_demo
    src/demo/Rot_demo.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(rot_demo glfw ${PLATFORM_LIBS})
set_output_dir(rot_demo)

add_executable(texture_demo
    src/demo/texture_demo.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(texture_demo glfw ${PLATFORM_LIBS})

add_executable(light_demo
    src/example/light.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(light_demo glfw ${PLATFORM_LIBS})
set_output_dir(light_demo)

add_executable(parallel_demo
    src/example/parallel.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(parallel_demo glfw ${PLATFORM_LIBS})
set_output_dir(parallel_demo)

add_executable(diffuse_demo
    src/example/light_example.cpp
    src/camera.cpp
    src/uni_light.cpp
    src/Graphics.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(diffuse_demo glfw ${ASSIMP_LINK_TARGET} ${ZLIB_LINK_TARGET} ${PLATFORM_LIBS})
set_output_dir(diffuse_demo)

add_executable(backpack_demo
    src/demo/backpack_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/modelload/Model.cpp
    includes/rendering/modelload/Mesh.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(backpack_demo glfw ${ASSIMP_LINK_TARGET} ${ZLIB_LINK_TARGET} ${PLATFORM_LIBS})
set_output_dir(backpack_demo)

add_executable(stencil_demo
    src/demo/stencil_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/assets/texture/Texture.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(stencil_demo glfw ${ASSIMP_LINK_TARGET} ${ZLIB_LINK_TARGET} ${PLATFORM_LIBS})
set_output_dir(stencil_demo)

add_executable(newbackpack
    src/demo/newbackpack.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/modelload/Model.cpp
    includes/rendering/modelload/Mesh.cpp
    ${GLAD_SOURCES}
    includes/rendering/assets/texture/HDRTexture.cpp
)
target_link_libraries(newbackpack glfw ${ASSIMP_LINK_TARGET} ${ZLIB_LINK_TARGET} ${PLATFORM_LIBS})
set_output_dir(newbackpack)

add_executable(fbo_demo
    src/demo/Frabuffer.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/core/EnvironmentController.cpp
    includes/rendering/debug/GpuProfiler.cpp
    includes/rendering/imgui/ui_import.cpp
    includes/rendering/modelload/Model.cpp
    includes/rendering/modelload/Mesh.cpp
    includes/rendering/assets/mesh/CubeMesh.cpp
    includes/rendering/assets/mesh/LightMesh.cpp
    includes/rendering/assets/mesh/PlaneMesh.cpp
    includes/rendering/assets/mesh/Screenquad.cpp
    includes/rendering/assets/mesh/SkyboxMesh.cpp
    includes/rendering/assets/mesh/SphereMesh.cpp
    includes/rendering/assets/mesh/TerrainMesh.cpp
    includes/rendering/assets/mesh/AlpineVegetationMeshFactory.cpp
    includes/rendering/assets/mesh/AlpineVegetationSystem.cpp
    includes/rendering/assets/mesh/WaterMesh.cpp
    includes/rendering/assets/texture/CubeMap.cpp
    includes/rendering/assets/texture/Texture.cpp
    includes/rendering/assets/texture/Material.cpp
    includes/rendering/core/ResourceRegistry.cpp
    includes/rendering/core/ModelDrawer.cpp
    includes/rendering/core/SceneRender.cpp
    includes/rendering/core/SphereDrawer.cpp
    includes/rendering/passes/debug/LightVisualPass.cpp
    includes/rendering/passes/debug/ShadowDebugPass.cpp
    includes/rendering/passes/deferred/DeferredRenderPass.cpp
    includes/rendering/passes/environment/SkyboxPass.cpp
    includes/rendering/passes/environment/CloudAccelerationPass.cpp
    includes/rendering/passes/environment/VolumetricSkyPass.cpp
    includes/rendering/passes/forward/ForwardHDRPass.cpp
    includes/rendering/passes/forward/ForwardOverlayPass.cpp
    includes/rendering/passes/forward/SceneObjectPass.cpp
    includes/rendering/passes/geometry/GeometryPass.cpp
    includes/rendering/passes/lighting/LightingPass.cpp
    includes/rendering/passes/lighting/VolumetricLightPass.cpp
    includes/rendering/passes/ssao/SSAOCommonPass.cpp
    includes/rendering/passes/shadow/DirectionalShadowPass.cpp
    includes/rendering/passes/shadow/PointShadowPass.cpp
    includes/rendering/passes/shadow/SpotShadowPass.cpp
    includes/rendering/postprocess/blur/BlurPass.cpp
    includes/rendering/postprocess/screen/ScreenPass.cpp
    includes/rendering/postprocess/taa/TemporalAAPass.cpp
    includes/rendering/resources/framebuffer/Gbuffer.cpp
    includes/rendering/resources/framebuffer/HDR_Framebuffer.cpp
    includes/rendering/resources/framebuffer/PingPong_Framebuffer.cpp
    includes/rendering/resources/framebuffer/SSAOPingPongFramebuffer.cpp
    includes/rendering/resources/framebuffer/TemporalAAFramebuffer.cpp
    includes/rendering/resources/render/SceneRenderResources.cpp
    includes/rendering/resources/ssao/SSAO.cpp
    includes/rendering/resources/scene/ScenePresets.cpp
    includes/rendering/resources/shader/ShaderLibrary.cpp
    includes/rendering/resources/shadow/DirectionalShadowMap.cpp
    includes/rendering/resources/shadow/PointShadowMap.cpp
    includes/rendering/resources/shadow/SpotShadowMap.cpp
    includes/rendering/uniforms/CameraUniformSetter.cpp
    includes/rendering/uniforms/LightUniformSetter.cpp
    includes/rendering/uniforms/RenderParams.cpp
    includes/rendering/uniforms/SSAOCameraUniformSetter.cpp
    includes/rendering/uniforms/SkyboxCameraUniformSetter.cpp
    ${GLAD_SOURCES}
    includes/rendering/assets/texture/HDRTexture.cpp
    includes/rendering/assets/texture/EnvCubemap.cpp
    includes/rendering/assets/ibl/IrradianceMap.cpp
    includes/rendering/assets/ibl/PrefilterMap.cpp
    includes/rendering/assets/ibl/BrdfLUT.cpp
    includes/rendering/core/RendererScene.cpp
)
target_link_libraries(fbo_demo imgui glfw ${ASSIMP_LINK_TARGET} ${ZLIB_LINK_TARGET} ${PLATFORM_LIBS})
target_compile_definitions(fbo_demo PRIVATE OPENGL_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
set_output_dir(fbo_demo)
