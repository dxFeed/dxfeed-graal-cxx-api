# Copyright (c) 2023 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.20)

project(console)
include(GNUInstallDirs)

set(TTLDTOR_CONSOLE "v1.0.1" CACHE STRING "The Console package version")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

# Determine if console is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED TTLDTOR_CONSOLE_ROOT_PROJECT)
    set(TTLDTOR_CONSOLE_ROOT_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(TTLDTOR_CONSOLE_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif ()
endif ()

option(TTLDTOR_CONSOLE_SAMPLES "Enable building samples for Console" ${TTLDTOR_CONSOLE_ROOT_PROJECT})

add_library(${PROJECT_NAME} src/console.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

add_library(console::console ALIAS ${PROJECT_NAME})

if (TTLDTOR_CONSOLE_SAMPLES)
    add_executable(${PROJECT_NAME}_test test/main.cpp)
    target_link_libraries(${PROJECT_NAME}_test console)
endif ()