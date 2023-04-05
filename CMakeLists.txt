# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.10)

project(dxFeedGraalCxxApi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

cmake_policy(SET CMP0135 NEW)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DXFC_IS_ROOT_PROJECT ON)
else()
    set(DXFC_ROOT_PROJECT OFF)
endif()

option(DXFC_BUILD_UNIT_TESTS "" DXFC_IS_ROOT_PROJECT)

option(DXFC_USE_DXFEED_GRAAL_NATIVE_API_GITHUB "" OFF)
set(DXFEED_GRAAL_NATIVE_API_GITHUB_VERSION "v1.0.5" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_API_GITHUB_ARCHIVE_VERSION "1.0.0" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_API_GITHUB_BASE_URL "https://github.com/ttldtor/dxfeed-graal-native-api/releases/download/" CACHE STRING "")

option(DXFC_USE_DXFEED_GRAAL_NATIVE_API_JFROG "" ON)
set(DXFEED_GRAAL_NATIVE_API_VERSION "0.4.0" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_API_JFROG_BASE_URL "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-api/" CACHE STRING "")

include(FetchContent)

if (DEFINED ENV{DXFEED_GRAAL_NATIVE_API_URL})
    FetchContent_Declare(DxFeedGraalNativeApi URL $ENV{DXFEED_GRAAL_NATIVE_API_URL})
else ()
    if (DXFC_USE_DXFEED_GRAAL_NATIVE_API_GITHUB)
        set(DXFEED_GRAAL_NATIVE_API_URL "${DXFEED_GRAAL_NATIVE_API_GITHUB_BASE_URL}${DXFEED_GRAAL_NATIVE_API_GITHUB_VERSION}/dxfeed-graal-native-api-${DXFEED_GRAAL_NATIVE_API_GITHUB_ARCHIVE_VERSION}")
    elseif (DXFC_USE_DXFEED_GRAAL_NATIVE_API_JFROG)
        set(DXFEED_GRAAL_NATIVE_API_URL "${DXFEED_GRAAL_NATIVE_API_JFROG_BASE_URL}${DXFEED_GRAAL_NATIVE_API_VERSION}/graal-native-api-${DXFEED_GRAAL_NATIVE_API_VERSION}")
    endif ()

    if (WIN32)
        set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-amd64-windows.zip)
    elseif (APPLE)
        if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
            set(DXFEED_GRAAL_NATIVE_API_URL ${DXFEED_GRAAL_NATIVE_API_URL}-aarch64-osx.zip)
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
endif ()

add_subdirectory(third_party/utfcpp-3.2.3)
add_subdirectory(third_party/fmt-9.1.0)

# find_package(utf8cpp)
# find_package(fmt)

set(dxFeedNativeAPIInternalSources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/Common.cpp
        )

set(dxFeedNativeAPIAPISources
        src/api/DXEndpoint.cpp
        src/api/DXFeed.cpp
        src/api/DXFeedSubscription.cpp
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

add_library(dxFeedGraalCxxApi
        ${dxFeedNativeAPIInternalSources}
        ${dxFeedNativeAPIAPISources}
        ${dxFeedNativeAPISystemSources}
        ${dxFeedNativeAPIEventSources}
        ${dxFeedNativeAPIEventMarketSources}
        ${dxFeedNativeAPIEventOptionSources}
        src/api.cpp
        )

target_include_directories(dxFeedGraalCxxApi PUBLIC include)
target_link_libraries(dxFeedGraalCxxApi PUBLIC DxFeedGraalNativeApi utf8cpp fmt::fmt)

add_custom_command(TARGET dxFeedGraalCxxApi POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeApi> $<TARGET_FILE_DIR:dxFeedGraalCxxApi>)

if (DXFC_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

add_subdirectory(samples/cpp/PrintQuoteEvents)
