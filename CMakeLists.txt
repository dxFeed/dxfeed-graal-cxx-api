# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.10)

project(dxFeedNativeAPI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

option(DXFC_BUILD_UNIT_TESTS "" ON)
option(DXFC_USE_CONAN "" ON)

include(FetchContent)

if (DEFINED ENV{DXFEED_GRAAL_NATIVE_API_URL})
    FetchContent_Declare(DxFeedGraalNativeApi URL $ENV{DXFEED_GRAAL_NATIVE_API_URL})
else ()
    set(DXFEED_GRAAL_NATIVE_API_GIT_VERSION "v1.0.3")
    set(DXFEED_GRAAL_NATIVE_API_ARCHIVE_VERSION "1.0.0")
    set(DXFEED_GRAAL_NATIVE_API_BASE_URL "https://github.com/ttldtor/dxfeed-graal-native-api/releases/download/")
    set(DXFEED_GRAAL_NATIVE_API_URL "${DXFEED_GRAAL_NATIVE_API_BASE_URL}${DXFEED_GRAAL_NATIVE_API_GIT_VERSION}/dxfeed-graal-native-api-${DXFEED_GRAAL_NATIVE_API_ARCHIVE_VERSION}")

    if (WIN32)
        set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-amd64-windows.zip)
    elseif (APPLE)
        if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
            set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-arm64-osx.zip)
        else ()
            set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-x86_64-osx.zip)
        endif ()
    elseif (UNIX)
        set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-amd64-linux.zip)
    else ()
        message(ERROR "Unknown platform!")
    endif ()

    FetchContent_Declare(DxFeedGraalNativeApi URL ${DXFEED_GRAAL_NATIVE_API_URL})
endif ()

FetchContent_MakeAvailable(DxFeedGraalNativeApi)
# DxFeedGraalNativeApi_SOURCE_DIR

if (DXFC_USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

find_package(utf8cpp)

add_library(dxFeedNativeAPI src/dxFeedNativeAPI.cpp)
target_include_directories(dxFeedNativeAPI PUBLIC include)
target_link_libraries(dxFeedNativeAPI PUBLIC DxFeedGraalNativeApi utf8cpp::utf8cpp)

add_library(dxFeedNativeCppAPI INTERFACE)
target_include_directories(dxFeedNativeCppAPI INTERFACE include)

add_custom_command(TARGET dxFeedNativeAPI POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeApi> $<TARGET_FILE_DIR:dxFeedNativeAPI>)

if (DXFC_BUILD_UNIT_TESTS)
    find_package(Catch2 3 REQUIRED)
    include(CTest)
    add_subdirectory(tests)
endif()

