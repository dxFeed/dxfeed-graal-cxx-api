# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.10)

project(PrintQuoteEvents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

cmake_policy(SET CMP0135 NEW)

add_executable(PrintQuoteEvents src/main.cpp)
target_include_directories(PrintQuoteEvents PUBLIC ../../../include)
target_link_libraries(PrintQuoteEvents PUBLIC DxFeedGraalNativeSdk dxFeedGraalCxxApi dxFeedGraalCxxApi_Clang)

add_custom_command(TARGET PrintQuoteEvents POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:PrintQuoteEvents>)