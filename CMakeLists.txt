cmake_minimum_required(VERSION 3.10)
project(OpenGLPractice)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_SOURCE_DIR}/third_party/stb
    ${CMAKE_SOURCE_DIR}/third_party/assimp/install/include
)

link_directories(${CMAKE_SOURCE_DIR}/third_party/assimp/install/lib)

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
target_link_libraries(diffuse_demo glfw assimp z ${PLATFORM_LIBS})
set_output_dir(diffuse_demo)

add_executable(backpack_demo
    src/demo/backpack_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/Model/Model.cpp
    includes/rendering/Model/Mesh.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(backpack_demo glfw assimp z ${PLATFORM_LIBS})
set_output_dir(backpack_demo)

add_executable(stencil_demo
    src/demo/stencil_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(stencil_demo glfw assimp z ${PLATFORM_LIBS})
set_output_dir(stencil_demo)

add_executable(newbackpack
    src/demo/newbackpack.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/Model/Model.cpp
    includes/rendering/Model/Mesh.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(newbackpack glfw assimp z ${PLATFORM_LIBS})
set_output_dir(newbackpack)

add_executable(fbo_demo
    src/demo/Frabuffer.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    includes/rendering/Model/Model.cpp
    includes/rendering/Model/Mesh.cpp
    includes/rendering/passes/SkyboxPass.cpp
    includes/rendering/passes/LightVisualPass.cpp
    includes/rendering/passes/SceneObjectPass.cpp
    includes/rendering/passes/GeometryPass.cpp
    includes/rendering/passes/LightingPass.cpp
    includes/rendering/passes/SSAOCommonPass.cpp
    includes/rendering/postprocess/SSAO.cpp
    includes/rendering/passes/ShadowPass/PointShadowPass.cpp
    ${GLAD_SOURCES}
)
target_link_libraries(fbo_demo imgui glfw assimp z ${PLATFORM_LIBS})
set_output_dir(fbo_demo)