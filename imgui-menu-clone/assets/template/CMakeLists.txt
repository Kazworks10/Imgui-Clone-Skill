cmake_minimum_required(VERSION 3.20)
project(imgui_menu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This template targets Win32 + D3D11.")
endif()

# ── Dear ImGui ───────────────────────────────────────────────────────────────
# Pinned on purpose: menu code is sensitive to API drift (BeginChild flags in
# 1.90, the font-atlas rework in 1.92). Bump deliberately, never implicitly.
set(UI_IMGUI_TAG "v1.91.9b" CACHE STRING "Dear ImGui git tag to build against")

include(FetchContent)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        ${UI_IMGUI_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)

# FreeType rasteriser. ImGui's built-in stb rasteriser renders lighter and
# softer; most real menus are built with FreeType, and matching text weight is
# one of the details that separates "close" from "identical". Costs ~1 min of
# first-build time, hence opt-in.
option(UI_USE_FREETYPE "Rasterise fonts with FreeType instead of stb" OFF)
if(UI_USE_FREETYPE)
    FetchContent_Declare(freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        VER-2-13-3
        GIT_SHALLOW    TRUE)
    set(FT_DISABLE_ZLIB   ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2  ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG    ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(freetype)
    list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp)
endif()

# ── target ───────────────────────────────────────────────────────────────────
add_executable(menu WIN32
    main.cpp
    ui/anim.cpp
    ui/widgets.cpp
    ui/shell_win32.cpp
    ${IMGUI_SOURCES})

target_include_directories(menu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)

target_compile_definitions(menu PRIVATE
    UNICODE _UNICODE
    NOMINMAX WIN32_LEAN_AND_MEAN
    IMGUI_DEFINE_MATH_OPERATORS)

target_link_libraries(menu PRIVATE d3d11 dxgi dwmapi)

if(UI_USE_FREETYPE)
    target_link_libraries(menu PRIVATE freetype)
    target_include_directories(menu PRIVATE ${imgui_SOURCE_DIR}/misc/freetype)
    target_compile_definitions(menu PRIVATE IMGUI_ENABLE_FREETYPE)
endif()

if(MSVC)
    target_compile_options(menu PRIVATE /W3 /permissive- /utf-8)
endif()

# Fonts are loaded from disk at runtime; keep them next to the binary.
add_custom_command(TARGET menu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:menu>/assets)
