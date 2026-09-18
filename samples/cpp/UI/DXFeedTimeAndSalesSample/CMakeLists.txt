# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(DXFeedTimeAndSalesSample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

option(DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND "Build the GLFW Wayland backend for UI samples" OFF)
if (UNIX AND NOT APPLE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND ${DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND} CACHE BOOL "" FORCE)
endif ()

set(DXFCXX_SAMPLE_GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/glfw-${GLFW_VERSION}")
if (EXISTS "${DXFCXX_SAMPLE_GLFW_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${DXFCXX_SAMPLE_GLFW_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/glfw")
else ()
    FetchContent_Declare(
            dxfcxx_sample_glfw
            URL https://github.com/glfw/glfw/archive/refs/tags/${GLFW_VERSION}.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(dxfcxx_sample_glfw)
    set(DXFCXX_SAMPLE_GLFW_SOURCE_DIR "${dxfcxx_sample_glfw_SOURCE_DIR}")
endif ()

set(DXFCXX_SAMPLE_IMGUI_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui-${IMGUI_VERSION}")
if (NOT EXISTS "${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/imgui.cpp")
    FetchContent_Declare(
            dxfcxx_sample_imgui
            URL https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(dxfcxx_sample_imgui)
    set(DXFCXX_SAMPLE_IMGUI_SOURCE_DIR "${dxfcxx_sample_imgui_SOURCE_DIR}")
endif ()

find_package(OpenGL REQUIRED)

add_library(DXFeedTimeAndSalesImGui STATIC
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/imgui.cpp
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(DXFeedTimeAndSalesImGui PUBLIC
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}
        ${DXFCXX_SAMPLE_IMGUI_SOURCE_DIR}/backends)
target_link_libraries(DXFeedTimeAndSalesImGui PUBLIC glfw OpenGL::GL)

add_executable(${PROJECT_NAME} src/main.cpp src/TimeAndSalesStore.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE DXFeedTimeAndSalesImGui fmt::fmt-header-only)

if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DXFCPP_USE_DLLS)
    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx::static)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND DXFCXX_LINK_STATIC_RUNTIME)
        set_target_properties(${PROJECT_NAME} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif ()
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx_mingw_debug_link_options_interface)

if (DXFCXX_FEATURE_STACKTRACE)
    LinkStacktrace(${PROJECT_NAME})
endif ()

if (DXFCXX_ENABLE_ASAN)
    LinkAsan(${PROJECT_NAME})
elseif (DXFCXX_ENABLE_UBSAN)
    LinkUbsan(${PROJECT_NAME})
endif ()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dxfcxx::graal>
        ${CMAKE_CURRENT_SOURCE_DIR}/dxfeed.system.properties
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (DXFCXX_BUILD_UNIT_TESTS)
    add_executable(DXFeedTimeAndSalesStoreTest tests/TimeAndSalesStoreTest.cpp)
    target_include_directories(DXFeedTimeAndSalesStoreTest PRIVATE
            src
            ${CMAKE_SOURCE_DIR}/third_party/doctest-${DOCTEST_VERSION})
    add_test(NAME DXFeedTimeAndSalesStoreTest COMMAND DXFeedTimeAndSalesStoreTest)
endif ()

if (DXFCXX_INSTALL AND DXFCXX_INSTALL_SAMPLES)
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR}/UI/${PROJECT_NAME})
    install(FILES dxfeed.system.properties DESTINATION ${CMAKE_INSTALL_BINDIR}/UI/${PROJECT_NAME})

    if (WIN32)
        install(FILES $<TARGET_FILE:dxfcxx::graal> DESTINATION ${CMAKE_INSTALL_BINDIR}/UI/${PROJECT_NAME})

        if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
            install(FILES $<TARGET_FILE:dxfcxx> DESTINATION ${CMAKE_INSTALL_BINDIR}/UI/${PROJECT_NAME})
        endif ()
    endif ()
endif ()
