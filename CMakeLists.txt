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
)

# GLAD 源文件
set(GLAD_SOURCES
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/gl.c
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

# # 添加可执行文件 - db_triangle
# add_executable(db_triangle
#     src/demo/db_triangle.cpp
#     ${GLAD_SOURCES}
# )
# 
# # 链接库
# target_link_libraries(db_triangle
#     glfw
#     "-framework OpenGL"
#     "-framework Cocoa"
#     "-framework IOKit"
#     "-framework CoreVideo"
# )
# 
# # 设置输出目录
# set_target_properties(db_triangle PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
# )

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

# 设置输出目录
set_target_properties(texture_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
