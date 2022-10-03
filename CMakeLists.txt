# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.10)

project(dxFeedNativeAPI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

add_library(dxFeedNativeAPI src/dxFeedNativeAPI.cpp)
target_include_directories(dxFeedNativeAPI PUBLIC include)
add_library(dxFeedNativeCppAPI INTERFACE)
target_include_directories(dxFeedNativeCppAPI INTERFACE include)
