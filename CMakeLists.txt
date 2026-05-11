cmake_minimum_required(VERSION 3.10)
project(OpenGLPractice)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 GLFW
find_package(glfw3 REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_SOURCE_DIR}/third_party/stb
    ${CMAKE_SOURCE_DIR}/third_party/assimp/install/include
)

# assimp 库目录
link_directories(${CMAKE_SOURCE_DIR}/third_party/assimp/install/lib)

# GLAD 源文件
set(GLAD_SOURCES
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/gl.c
)

# 添加可执行文件 - 3Ddemo
add_executable(3Ddemo
    src/demo/3Ddemo.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(3Ddemo
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(3Ddemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - 3Dbkp
add_executable(3Dbkp
    src/example/3Dbkp.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(3Dbkp
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(3Dbkp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - Rot_demo
add_executable(rot_demo
    src/demo/Rot_demo.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(rot_demo
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(rot_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)


# 添加可执行文件 - texture_demo
add_executable(texture_demo
    src/demo/texture_demo.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(texture_demo
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 添加可执行文件 - light_demo
add_executable(light_demo
    src/example/light.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(light_demo
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(light_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - parallel_demo
add_executable(parallel_demo
    src/example/parallel.cpp
    src/camera.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(parallel_demo
    glfw
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(parallel_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)


# 添加可执行文件 - diffuse_demo
add_executable(diffuse_demo
    src/example/light_example.cpp
    src/camera.cpp
    src/uni_light.cpp
    src/Graphics.cpp
    ${GLAD_SOURCES}
)
# 链接库
target_link_libraries(diffuse_demo
    glfw
    assimp
    z
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(diffuse_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - backpack_demo
add_executable(backpack_demo
    src/demo/backpack_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(backpack_demo
    glfw
    assimp
    z
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(backpack_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - stencil_demo
add_executable(stencil_demo
    src/demo/stencil_demo.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(stencil_demo
    glfw
    assimp
    z
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(stencil_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - newbackpack
add_executable(newbackpack
    src/demo/newbackpack.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(newbackpack
    glfw
    assimp
    z
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(newbackpack PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# 添加可执行文件 - fbo_demo
add_executable(fbo_demo
    src/demo/Frabuffer.cpp
    src/camera.cpp
    src/stb_image_impl.cpp
    ${GLAD_SOURCES}
)

# 链接库
target_link_libraries(fbo_demo
    glfw
    assimp
    z
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

# 设置输出目录
set_target_properties(fbo_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
