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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DXFC_IS_ROOT_PROJECT ON)
else ()
    set(DXFC_ROOT_PROJECT OFF)
endif ()

option(DXFC_BUILD_UNIT_TESTS "" ON)
option(DXFC_BUILD_SHARED "" ON)
option(DXFC_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_VERSION "1.0.0" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/" CACHE STRING "")

include(FetchContent)

if (DEFINED ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
    FetchContent_Declare(DxFeedGraalNativeSdk URL $ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
else ()
    if (DXFC_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG)
        set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL}${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
    endif ()

    if (WIN32)
        set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-amd64-windows.zip)
    elseif (APPLE)
        if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
            set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-aarch64-osx.zip)
        else ()
            set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-x86_64-osx.zip)
        endif ()
    elseif (UNIX)
        set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-amd64-linux.zip)
    else ()
        message(ERROR "Unknown platform!")
    endif ()

    FetchContent_Declare(DxFeedGraalNativeSdk URL ${DXFEED_GRAAL_NATIVE_SDK_URL})
endif ()

FetchContent_MakeAvailable(DxFeedGraalNativeSdk)
# DxFeedGraalNativeSdk_SOURCE_DIR

if (DXFC_USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif ()

add_subdirectory(third_party/utfcpp-3.2.3)
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


add_library(dxFeedGraalCxxApi_static STATIC
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

add_library(dxFeedGraalCxxApi SHARED
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

#set_target_properties(dxFeedGraalCxxApi PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(dxFeedGraalCxxApi_CompilerOptions INTERFACE)

if ((${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang") OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "-Wall" "-Wextra" "-Wpedantic" "-Werror")
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "-Wno-undefined-var-template" "-Wno-unused-private-field")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "-Wall" "-Wextra" "-Wpedantic" "-Werror")
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "-fPIC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "/W4" "/WX")
    target_compile_options(dxFeedGraalCxxApi_CompilerOptions INTERFACE "/wd4996" "/wd4251")
endif ()

target_include_directories(dxFeedGraalCxxApi_static PUBLIC include)
target_include_directories(dxFeedGraalCxxApi PUBLIC include)
target_compile_definitions(dxFeedGraalCxxApi PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS)

target_link_libraries(dxFeedGraalCxxApi_static PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt PUBLIC dxFeedGraalCxxApi_CompilerOptions)
target_link_libraries(dxFeedGraalCxxApi PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt PUBLIC dxFeedGraalCxxApi_CompilerOptions)

add_custom_command(TARGET dxFeedGraalCxxApi_static POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:dxFeedGraalCxxApi_static>)
add_custom_command(TARGET dxFeedGraalCxxApi POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:dxFeedGraalCxxApi>)

if (DXFC_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

add_subdirectory(samples/cpp/PrintQuoteEvents)
add_subdirectory(tools/Tools)
