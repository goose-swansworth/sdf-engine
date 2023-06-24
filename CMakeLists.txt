cmake_minimum_required(VERSION 3.0.0)
project(compute_shaders)
cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

add_executable(compute_shaders src/main.cpp src/glad.c)

target_link_libraries(compute_shaders
    glfw
    OpenGL::GL
)

