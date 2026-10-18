# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.4)
project(PortalsModuleSpike LANGUAGES CXX)

include(CTest)
option(SPIKE_BUILD_MODULES "Build the named-module experiment and its consumers" ON)
option(SPIKE_CHECK_IMPORT_FIRST "Probe import-before-header, unsupported by some toolchains" OFF)
option(SPIKE_BUILD_DOCS "Build the toy API reference with Doxygen" OFF)

add_library(toy_shared SHARED src/api.cpp)
target_compile_features(toy_shared PUBLIC cxx_std_23)
set_target_properties(toy_shared PROPERTIES CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_compile_definitions(toy_shared PRIVATE TOY_BUILDING_LIBRARY)
target_sources(toy_shared PUBLIC FILE_SET HEADERS BASE_DIRS include FILES include/toy/api.hpp)

if(SPIKE_BUILD_MODULES)
  add_library(toy_module STATIC)
  target_link_libraries(toy_module PUBLIC toy_shared)
  target_sources(toy_module PUBLIC FILE_SET CXX_MODULES FILES src/toy.cppm)
  set_target_properties(toy_module PROPERTIES CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
endif()

function(spike_consumer name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE toy_shared)
  set_target_properties(${name} PROPERTIES CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES TIMEOUT 10)
endfunction()

spike_consumer(header_consumer checks/header.cpp)
if(SPIKE_BUILD_MODULES)
  spike_consumer(import_consumer checks/import.cpp)
  spike_consumer(mixed_consumer checks/mixed.cpp checks/header_peer.cpp)
  target_link_libraries(import_consumer PRIVATE toy_module)
  target_link_libraries(mixed_consumer PRIVATE toy_module)
  if(SPIKE_CHECK_IMPORT_FIRST)
    spike_consumer(import_first_consumer checks/import_first.cpp checks/header_peer.cpp)
    target_link_libraries(import_first_consumer PRIVATE toy_module)
  endif()
endif()

if(SPIKE_BUILD_DOCS)
  find_package(Doxygen REQUIRED)
  set(DOXYGEN_EXTRACT_ALL NO)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_WARN_AS_ERROR YES)
  doxygen_add_docs(spike_docs include/toy/api.hpp COMMENT "Build the toy API reference")
endif()
