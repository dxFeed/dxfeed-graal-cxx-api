# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(WriteTapeFileSample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path;@executable_path;@executable_path/../Frameworks")
elseif (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib64:$ORIGIN/../lib:$ORIGIN")
endif ()

add_executable(${PROJECT_NAME} src/main.cpp)

if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DXFCPP_USE_DLLS)
    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx::static)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if (DXFCXX_LINK_STATIC_RUNTIME)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            )
        endif ()
    endif ()
endif ()

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
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif ()

if (DXFCXX_INSTALL AND DXFCXX_INSTALL_SAMPLES)
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

    if (WIN32)
        install(FILES $<TARGET_FILE:dxfcxx::graal> DESTINATION ${CMAKE_INSTALL_BINDIR})

        if (DXFCXX_DYNAMICALLY_LINK_SAMPLES)
            install(FILES $<TARGET_FILE:dxfcxx> DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif ()
    endif ()
endif ()