# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(portals)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

if (NOT DEFINED ORG_TTLDTOR_PORTALS_IS_ROOT_PROJECT)
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(ORG_TTLDTOR_PORTALS_IS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else ()
        set(ORG_TTLDTOR_PORTALS_IS_ROOT_PROJECT OFF)
    endif ()
endif ()

option(ORG_TTLDTOR_PORTALS_BUILD_SAMPLES "Build samples" ${ORG_TTLDTOR_PORTALS_IS_ROOT_PROJECT})

add_library(${PROJECT_NAME} src/portals.cpp include/portals.hpp)
add_library(portals::portals ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC include)

if (ORG_TTLDTOR_PORTALS_BUILD_SAMPLES)
    add_subdirectory(samples/Simple)
endif ()