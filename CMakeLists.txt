cmake_minimum_required(VERSION 3.0.0)
project(compute_shaders)
cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(src/glfw)


add_executable(compute_shaders src/main.cpp src/stb.cpp src/glad.c src/vera/glob.cpp src/vera/string.cpp src/vera/fs.cpp)

target_link_libraries(compute_shaders
    glfw
    OpenGL::GL
)

