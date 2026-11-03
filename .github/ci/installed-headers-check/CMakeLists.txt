# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks that the installation has no internal headers and no third-party headers, then compiles every installed C++
# header of the library on its own, with the include directory of the installation as the only include directory: a
# header that needs a third-party library or a header that is not installed fails here.
# On Windows each header is also compiled after <windows.h> (without NOMINMAX).
# Usage: cmake -S .github/ci/installed-headers-check -B <dir> -DDXFCXX_INSTALL_PREFIX=<prefix>
#        cmake --build <dir>

cmake_minimum_required(VERSION 3.21)

project(dxfcxx_installed_headers_check LANGUAGES CXX)

set(DXFCXX_INSTALL_PREFIX "" CACHE PATH "The prefix the library was installed to (cmake --install)")

set(DXFCXX_INSTALLED_INCLUDE_DIR "${DXFCXX_INSTALL_PREFIX}/include")

if (NOT EXISTS "${DXFCXX_INSTALLED_INCLUDE_DIR}/dxfeed_graal_cpp_api/api.hpp")
    message(FATAL_ERROR "No installed C++ API headers in '${DXFCXX_INSTALLED_INCLUDE_DIR}': set DXFCXX_INSTALL_PREFIX")
endif ()

# The internal headers stay in src/ and the private dependencies (bits) are not installed.
foreach (forbidden dxfeed_graal_cpp_api/internal dxfeed_graal_cpp_api/isolated bits)
    if (EXISTS "${DXFCXX_INSTALLED_INCLUDE_DIR}/${forbidden}")
        message(FATAL_ERROR "'${DXFCXX_INSTALLED_INCLUDE_DIR}/${forbidden}' must not be installed")
    endif ()
endforeach ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE DXFCXX_INSTALLED_HEADERS RELATIVE "${DXFCXX_INSTALLED_INCLUDE_DIR}"
        "${DXFCXX_INSTALLED_INCLUDE_DIR}/dxfeed_graal_cpp_api/*.hpp")

set(DXFCXX_CHECK_SOURCES "")

foreach (header ${DXFCXX_INSTALLED_HEADERS})
    string(MAKE_C_IDENTIFIER "${header}" headerId)

    set(source "${CMAKE_CURRENT_BINARY_DIR}/sources/${headerId}.cpp")
    file(CONFIGURE OUTPUT "${source}" CONTENT "#include <${header}>\n")
    list(APPEND DXFCXX_CHECK_SOURCES "${source}")

    if (WIN32)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/sources/${headerId}_after_windows_h.cpp")
        file(CONFIGURE OUTPUT "${source}" CONTENT "#include <windows.h>\n#include <${header}>\n")
        list(APPEND DXFCXX_CHECK_SOURCES "${source}")
    endif ()
endforeach ()

list(LENGTH DXFCXX_INSTALLED_HEADERS DXFCXX_INSTALLED_HEADERS_COUNT)
message(STATUS "Installed C++ headers to check: ${DXFCXX_INSTALLED_HEADERS_COUNT}")

add_library(dxfcxx_installed_headers_check OBJECT ${DXFCXX_CHECK_SOURCES})
target_include_directories(dxfcxx_installed_headers_check PRIVATE "${DXFCXX_INSTALLED_INCLUDE_DIR}")

if (MSVC)
    target_compile_options(dxfcxx_installed_headers_check PRIVATE /EHsc /utf-8 /permissive- /Zc:__cplusplus)
endif ()
