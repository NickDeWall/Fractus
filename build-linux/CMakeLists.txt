cmake_minimum_required(VERSION 3.10)

project(fractus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

set(ROOT ${PROJECT_SOURCE_DIR}/..)
set(IMGUI ${ROOT}/third_party/imgui)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET sdl2 SDL2_ttf glew)

add_executable(fractus
    ${ROOT}/src/main.cpp
    ${ROOT}/src/core/app.cpp
    ${ROOT}/src/core/window.cpp
    ${ROOT}/src/input/input_manager.cpp
    ${ROOT}/src/render/collision_field.cpp
    ${ROOT}/src/render/fractal_manager.cpp
    ${ROOT}/src/render/display_modes.cpp
    ${ROOT}/src/render/overlay_renderer.cpp
    ${ROOT}/src/render/shader_manager.cpp
    ${ROOT}/src/scene/screen.cpp
    ${ROOT}/src/scene/screen_manager.cpp
    ${ROOT}/src/ui/ui_manager.cpp
    ${ROOT}/src/util/math_utils.cpp
    ${IMGUI}/imgui.cpp
    ${IMGUI}/imgui_draw.cpp
    ${IMGUI}/imgui_tables.cpp
    ${IMGUI}/imgui_widgets.cpp
    ${IMGUI}/imgui_impl_sdl2.cpp
    ${IMGUI}/imgui_impl_opengl3.cpp
)

target_include_directories(fractus PRIVATE
    ${ROOT}/src
    ${IMGUI}
)

target_link_libraries(fractus PRIVATE
    PkgConfig::DEPS
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)