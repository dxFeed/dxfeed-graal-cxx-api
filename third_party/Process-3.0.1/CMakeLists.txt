# Copyright (c) 2023 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.20)

project(process)
include(GNUInstallDirs)

set(TTLDTOR_PROCESS "v3.0.1" CACHE STRING "The Process package version")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

# Determine if process is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED TTLDTOR_PROCESS_ROOT_PROJECT)
    set(TTLDTOR_PROCESS_ROOT_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(TTLDTOR_PROCESS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif ()
endif ()

option(TTLDTOR_PROCESS_SAMPLES "Enable building samples for Process" ${TTLDTOR_PROCESS_ROOT_PROJECT})

add_library(${PROJECT_NAME} src/process.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

add_library(process::process ALIAS ${PROJECT_NAME})

if (WIN32)
    target_link_libraries(process PRIVATE psapi)
else ()
    message(STATUS "Platform: " ${CMAKE_SYSTEM_NAME})
endif ()

if (TTLDTOR_PROCESS_SAMPLES)
    add_executable(${PROJECT_NAME}_test test/main.cpp)
    target_link_libraries(${PROJECT_NAME}_test process)
endif ()