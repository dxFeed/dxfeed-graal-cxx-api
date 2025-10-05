# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.16)
project(config VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(config INTERFACE)
add_library(config::config ALIAS config)
add_library(org::ttldtor::config::config ALIAS config)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets)
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Config.cmake
    NAMESPACE config::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

if (NOT DEFINED ORG_TTLDTOR_CONFIG_IS_ROOT_PROJECT)
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(ORG_TTLDTOR_CONFIG_IS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else ()
        set(ORG_TTLDTOR_CONFIG_ROOT_PROJECT OFF)
    endif ()
endif ()

option(ORG_TTLDTOR_CONFIG_BUILD_UNIT_TESTS "Build tests" ${ORG_TTLDTOR_CONFIG_IS_ROOT_PROJECT})
if (ORG_TTLDTOR_CONFIG_BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(ORG_TTLDTOR_CONFIG_BUILD_EXAMPLES "Build examples" ${ORG_TTLDTOR_CONFIG_IS_ROOT_PROJECT})
if (ORG_TTLDTOR_CONFIG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()