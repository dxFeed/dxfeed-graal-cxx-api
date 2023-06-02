# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(dxFeedGraalCxxApi)

set(DXFCXX_VERSION "0.0.0-ALPHA")
set(DXFCXX_PACKAGE_VERSION "0.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

if (NOT DEFINED DXFCXX_IS_ROOT_PROJECT)
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(DXFCXX_IS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else ()
        set(DXFCXX_ROOT_PROJECT OFF)
    endif ()
endif ()

option(DXFCXX_BUILD_UNIT_TESTS "Build tests" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_BUILD_SAMPLES "Build samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_BUILD_TOOLS "Build tools" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL "Prepare install" ON)
option(DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_VERSION "1.0.0" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/" CACHE STRING "")

include(FetchContent)

if (WIN32)
    set(DXFCXX_TARGET_PLATFORM "amd64-windows")
elseif (APPLE)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
        set(DXFCXX_TARGET_PLATFORM "aarch64-osx")
    else ()
        set(DXFCXX_TARGET_PLATFORM "x86_64-osx")
    endif ()
elseif (UNIX)
    set(DXFCXX_TARGET_PLATFORM "amd64-linux")
elseif ()
    set(DXFCXX_TARGET_PLATFORM "unknown")
endif ()

if (DEFINED ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
    FetchContent_Declare(DxFeedGraalNativeSdk URL $ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
else ()
    if (DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG)
        set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL}${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
    endif ()

    if (DXFCXX_TARGET_PLATFORM STREQUAL "unknown")
        message(ERROR "Unknown platform!")
    else ()
        set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-${DXFCXX_TARGET_PLATFORM}.zip)
    endif ()

    FetchContent_Declare(DxFeedGraalNativeSdk URL ${DXFEED_GRAAL_NATIVE_SDK_URL})
endif ()

FetchContent_MakeAvailable(DxFeedGraalNativeSdk)
# DxFeedGraalNativeSdk_SOURCE_DIR

add_subdirectory(third_party/utfcpp-3.2.3)
set(FMT_INSTALL OFF)
add_subdirectory(third_party/fmt-10.0.0)

# find_package(utf8cpp)
# find_package(fmt)

set(dxFeedNativeAPIInternalSources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/JavaObjectHandler.cpp
        src/internal/EventClassList.cpp
        src/internal/SymbolList.cpp
        src/internal/Common.cpp
        )

set(dxFeedNativeAPIInternalUtilsSources
        src/internal/utils/StringUtils.cpp
        )

set(dxFeedNativeAPIInternalUtilsDebugSources
        src/internal/utils/debug/Debug.cpp
        )

set(dxFeedNativeAPIAPISources
        src/api/DXEndpoint.cpp
        src/api/DXFeed.cpp
        src/api/DXFeedSubscription.cpp
        )

set(dxFeedNativeAPIAPIOsubSources
        src/api/osub/WildcardSymbol.cpp
        )

set(dxFeedNativeAPISymbolsSources
        src/symbols/SymbolMapper.cpp
        src/symbols/StringSymbol.cpp
        src/symbols/SymbolWrapper.cpp
        )

set(dxFeedNativeAPISystemSources
        src/system/System.cpp
        )

set(dxFeedNativeAPIEventSources
        src/event/EventTypeEnum.cpp
        src/event/IndexedEventSource.cpp
        src/event/EventFlag.cpp
        src/event/IndexedEvent.cpp
        src/event/EventMapper.cpp
        )

set(dxFeedNativeAPIEventMarketSources
        src/event/market/Quote.cpp
        src/event/market/ShortSaleRestriction.cpp
        src/event/market/TradingStatus.cpp
        src/event/market/Profile.cpp
        src/event/market/PriceType.cpp
        src/event/market/Summary.cpp
        src/event/market/Direction.cpp
        src/event/market/TradeBase.cpp
        src/event/market/Trade.cpp
        src/event/market/TradeETH.cpp
        src/event/market/Side.cpp
        src/event/market/TimeAndSaleType.cpp
        src/event/market/TimeAndSale.cpp
        src/event/market/OptionSale.cpp
        )

set(dxFeedNativeAPIEventOptionSources
        src/event/option/Greeks.cpp
        src/event/option/Underlying.cpp
        src/event/option/TheoPrice.cpp
        src/event/option/Series.cpp
        )


add_library(${PROJECT_NAME}_static STATIC
        ${dxFeedNativeAPIInternalSources}
        ${dxFeedNativeAPIInternalUtilsSources}
        ${dxFeedNativeAPIInternalUtilsDebugSources}
        ${dxFeedNativeAPIAPISources}
        ${dxFeedNativeAPIAPIOsubSources}
        ${dxFeedNativeAPISymbolsSources}
        ${dxFeedNativeAPISystemSources}
        ${dxFeedNativeAPIEventSources}
        ${dxFeedNativeAPIEventMarketSources}
        ${dxFeedNativeAPIEventOptionSources}
        src/api.cpp
        )

add_library(${PROJECT_NAME} SHARED
        ${dxFeedNativeAPIInternalSources}
        ${dxFeedNativeAPIInternalUtilsSources}
        ${dxFeedNativeAPIInternalUtilsDebugSources}
        ${dxFeedNativeAPIAPISources}
        ${dxFeedNativeAPIAPIOsubSources}
        ${dxFeedNativeAPISymbolsSources}
        ${dxFeedNativeAPISystemSources}
        ${dxFeedNativeAPIEventSources}
        ${dxFeedNativeAPIEventMarketSources}
        ${dxFeedNativeAPIEventOptionSources}
        src/api.cpp
        )

add_library(dxfcxx ALIAS ${PROJECT_NAME})
add_library(dxfcxx::static ALIAS ${PROJECT_NAME}_static)
add_library(dxfcxx::graal ALIAS DxFeedGraalNativeSdk)

set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(${PROJECT_NAME}_CompilerOptions INTERFACE)

if ((${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang") OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "-Wall" "-Wextra" "-Wpedantic" "-Werror")
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "-Wno-undefined-var-template" "-Wno-unused-private-field")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "-Wall" "-Wextra" "-Wpedantic" "-Werror")
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "-fPIC" "-no-pie")
    target_link_options(${PROJECT_NAME}_CompilerOptions INTERFACE "-fPIC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "/W4" "/WX")
    target_compile_options(${PROJECT_NAME}_CompilerOptions INTERFACE "/wd4996" "/wd4251")
endif ()

add_library(${PROJECT_NAME}_Shared INTERFACE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(${PROJECT_NAME}_Shared PROPERTIES
            CMAKE_MACOSX_RPATH ON
            CMAKE_SKIP_BUILD_RPATH ON
            CMAKE_BUILD_WITH_INSTALL_RPATH ON
            CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF
            CMAKE_BUILD_RPATH_USE_ORIGIN ON
            CMAKE_INSTALL_RPATH "@loader_path"
            )
elseif (UNIX)
    set_target_properties(${PROJECT_NAME}_Shared PROPERTIES
            CMAKE_SKIP_BUILD_RPATH ON
            CMAKE_BUILD_WITH_INSTALL_RPATH ON
            CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF
            CMAKE_BUILD_RPATH_USE_ORIGIN ON
            CMAKE_INSTALL_RPATH "$ORIGIN"
            )
endif ()

target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE
        FMT_HEADER_ONLY=1)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS FMT_HEADER_ONLY=1)

target_link_libraries(${PROJECT_NAME}_static PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only PUBLIC ${PROJECT_NAME}_CompilerOptions)
target_link_libraries(${PROJECT_NAME} PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only PUBLIC ${PROJECT_NAME}_CompilerOptions ${PROJECT_NAME}_Shared)

add_custom_command(TARGET ${PROJECT_NAME}_static POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}_static>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if (DXFCXX_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (DXFCXX_BUILD_SAMPLES)
    add_subdirectory(samples/cpp/PrintQuoteEvents)
    add_subdirectory(samples/cpp/DxFeedSample)
endif ()

if (DXFCXX_BUILD_TOOLS)
    add_subdirectory(tools/Tools)
endif ()

if (DXFCXX_INSTALL)
    set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}PackConfig.cmake")
    set(CPACK_SOURCE_GENERATOR ZIP)
    set(CPACK_PACKAGE_VENDOR "Devexperts LLC")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${DXFCXX_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${DXFCXX_VERSION}-${DXFCXX_TARGET_PLATFORM}${PACKAGE_SUFFIX}")
    include(CPack)

    install(TARGETS ${PROJECT_NAME} DESTINATION "bin")
    install(TARGETS ${PROJECT_NAME}_static DESTINATION "bin")
    install(DIRECTORY "include" DESTINATION "." PATTERN "build" EXCLUDE)
    install(FILES "LICENSE" "README.md" DESTINATION ".")
endif ()