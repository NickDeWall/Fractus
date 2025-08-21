
cmake_minimum_required(VERSION 3.10)

project(fractus2)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

add_executable(fractus
    ${PROJECT_SOURCE_DIR}/../src/main.cpp
    ${PROJECT_SOURCE_DIR}/../src/fractal_manager.cpp
    ${PROJECT_SOURCE_DIR}/../src/math_utils.cpp
    ${PROJECT_SOURCE_DIR}/../src/screen.cpp
    ${PROJECT_SOURCE_DIR}/../src/screen_manager.cpp
    ${PROJECT_SOURCE_DIR}/../src/shader_manager.cpp
)

target_include_directories(fractus PUBLIC
    ${PROJECT_SOURCE_DIR}/../header
)

target_link_libraries(fractus PUBLIC
    GLEW
    GL
    SDL2
)

