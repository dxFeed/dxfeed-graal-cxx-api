# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

project(dxFeedGraalCxxApi_Internal)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

set(dxFeedGraalCxxApi_Internal_Sources
        CEntryPointErrors.cpp
        Isolate.cpp
        JavaObjectHandler.cpp
        EventClassList.cpp
        SymbolList.cpp
        Common.cpp
)

set(dxFeedGraalCxxApi_Internal_Utils_Sources
        utils/StringUtils.cpp
        utils/EnumUtils.cpp
        utils/CmdArgsUtils.cpp
)

set(dxFeedGraalCxxApi_Internal_UtilsDebug_Sources
        utils/debug/Debug.cpp
)


add_library(dxFeedGraalCxxApi_Internal OBJECT
        ${dxFeedGraalCxxApi_Internal_Sources}
        ${dxFeedGraalCxxApi_Internal_Utils_Sources}
        ${dxFeedGraalCxxApi_Internal_UtilsDebug_Sources}
)

add_library(dxfcxx::internal ALIAS dxFeedGraalCxxApi_Internal)

target_include_directories(dxFeedGraalCxxApi_Internal PRIVATE ../../include ../../third_party/range-v3-0.12/include)
target_link_libraries(dxFeedGraalCxxApi_Internal PRIVATE DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only)