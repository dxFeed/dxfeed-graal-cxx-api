# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(Tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_executable(Tools src/main.cpp)
target_include_directories(Tools PRIVATE ../../include/dxfeed_graal_cpp_api)
target_link_libraries(Tools PRIVATE DxFeedGraalNativeSdk dxFeedGraalCxxApi_static dxFeedGraalCxxApi_CompilerOptions)

add_custom_command(TARGET Tools POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE:dxFeedGraalCxxApi_static> $<TARGET_FILE_DIR:Tools>)