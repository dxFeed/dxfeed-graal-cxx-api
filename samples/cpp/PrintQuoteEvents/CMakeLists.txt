# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(PrintQuoteEvents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_executable(PrintQuoteEvents src/main.cpp)
target_include_directories(PrintQuoteEvents PRIVATE ../../../include)

target_link_libraries(PrintQuoteEvents PRIVATE DxFeedGraalNativeSdk dxFeedGraalCxxApi dxFeedGraalCxxApi_CompilerOptions)
target_compile_definitions(PrintQuoteEvents PRIVATE DXFCPP_USE_DLLS)

add_custom_command(TARGET PrintQuoteEvents POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE:dxFeedGraalCxxApi> $<TARGET_FILE_DIR:PrintQuoteEvents>)

add_custom_command(TARGET PrintQuoteEvents POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/dxfeed.properties $<TARGET_FILE_DIR:PrintQuoteEvents>)