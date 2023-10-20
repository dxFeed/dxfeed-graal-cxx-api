# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(dxFeedGraalCxxApi)

set(DXFCXX_VERSION "v0.3.0-alpha" CACHE STRING "The dxFeed Graal CXX API package version")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
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
option(DXFCXX_INSTALL_LIB "Prepare install the libraries" ON)
option(DXFCXX_INSTALL_SAMPLES "Prepare install the samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL_TOOLS "Prepare install the tools" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_VERSION "1.0.6" CACHE STRING "")
set(DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/" CACHE STRING "")

option(DXFCXX_ENABLE_ASAN_UBSAN "Enable address, UB sanitizers etc" OFF)
option(DXFCXX_ENABLE_VS_ASAN "Enable VS address sanitizer" OFF)
option(DXFCXX_USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)

include(FetchContent)

set(DXFCXX_GRAAL_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_TARGET_CPU "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_CPU "unknown" CACHE STRING "")

include(ParseAndDetectPlatforms.cmake)

if (DEFINED ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
    FetchContent_Declare(DxFeedGraalNativeSdk URL $ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
else ()
    if (DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG)
        set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL}${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
    endif ()

    if (DXFCXX_GRAAL_TARGET_PLATFORM STREQUAL "unknown-unknown")
        message(ERROR "Unsupported platform!")
    else ()
        set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-${DXFCXX_GRAAL_TARGET_PLATFORM}.zip)
    endif ()

    FetchContent_Declare(DxFeedGraalNativeSdk URL ${DXFEED_GRAAL_NATIVE_SDK_URL})
endif ()

FetchContent_MakeAvailable(DxFeedGraalNativeSdk)
# DxFeedGraalNativeSdk_SOURCE_DIR

FetchContent_Declare(Process GIT_REPOSITORY "https://github.com/ttldtor/Process.git" GIT_TAG default)
FetchContent_MakeAvailable(Process)

FetchContent_Declare(Console GIT_REPOSITORY "https://github.com/ttldtor/Console.git" GIT_TAG default)
FetchContent_MakeAvailable(Console)

add_subdirectory(third_party/utfcpp-3.2.3)
set(FMT_INSTALL OFF)
add_subdirectory(third_party/fmt-10.0.0)
#set(BUILD_TZ_LIB ON)
#set(USE_SYSTEM_TZ_DB ON)
add_subdirectory(third_party/date-3.0.1)

# find_package(utf8cpp)
# find_package(fmt)

set(dxFeedGraalCxxApi_Internal_Sources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/JavaObjectHandle.cpp
        src/internal/EventClassList.cpp
        src/internal/SymbolList.cpp
        src/internal/Common.cpp
        src/internal/Platform.cpp
)

set(dxFeedGraalCxxApi_InternalUtils_Sources
        src/internal/utils/StringUtils.cpp
        src/internal/utils/EnumUtils.cpp
        src/internal/utils/TimeFormat.cpp
        src/internal/utils/CmdArgsUtils.cpp
)

set(dxFeedGraalCxxApi_InternalUtilsDebug_Sources
        src/internal/utils/debug/Debug.cpp
)

set(dxFeedGraalCxxApi_Api_Sources
        src/api/DXEndpoint.cpp
        src/api/DXFeed.cpp
        src/api/DXFeedSubscription.cpp
        src/api/DXPublisher.cpp
)

set(dxFeedGraalCxxApi_ApiOsub_Sources
        src/api/osub/WildcardSymbol.cpp
        src/api/osub/TimeSeriesSubscriptionSymbol.cpp
        src/api/osub/IndexedEventSubscriptionSymbol.cpp
)

set(dxFeedGraalCxxApi_Ipf_Sources
        src/ipf/InstrumentProfile.cpp
        src/ipf/InstrumentProfileType.cpp
        src/ipf/InstrumentProfileField.cpp
        src/ipf/InstrumentProfileReader.cpp
        src/ipf/live/InstrumentProfileCollector.cpp
        src/ipf/live/InstrumentProfileConnection.cpp
)

set(dxFeedGraalCxxApi_Symbols_Sources
        src/symbols/StringSymbol.cpp
        src/symbols/SymbolWrapper.cpp
)

set(dxFeedGraalCxxApi_System_Sources
        src/system/System.cpp
)

set(dxFeedGraalCxxApi_Event_Sources
        src/event/EventTypeEnum.cpp
        src/event/IndexedEventSource.cpp
        src/event/EventFlag.cpp
        src/event/IndexedEvent.cpp
        src/event/EventMapper.cpp
)

set(dxFeedGraalCxxApi_EventCandle_Sources
        src/event/candle/Candle.cpp
        src/event/candle/CandleAlignment.cpp
        src/event/candle/CandleExchange.cpp
        src/event/candle/CandlePeriod.cpp
        src/event/candle/CandlePrice.cpp
        src/event/candle/CandlePriceLevel.cpp
        src/event/candle/CandleType.cpp
        src/event/candle/CandleSession.cpp
        src/event/candle/CandleSymbol.cpp
)

set(dxFeedGraalCxxApi_EventMarket_Sources
        src/event/market/AnalyticOrder.cpp
        src/event/market/Direction.cpp
        src/event/market/IcebergType.cpp
        src/event/market/MarketEvent.cpp
        src/event/market/OptionSale.cpp
        src/event/market/Order.cpp
        src/event/market/OrderAction.cpp
        src/event/market/OrderBase.cpp
        src/event/market/OrderSource.cpp
        src/event/market/PriceType.cpp
        src/event/market/Profile.cpp
        src/event/market/Quote.cpp
        src/event/market/Scope.cpp
        src/event/market/ShortSaleRestriction.cpp
        src/event/market/Side.cpp
        src/event/market/SpreadOrder.cpp
        src/event/market/Summary.cpp
        src/event/market/TimeAndSale.cpp
        src/event/market/TimeAndSaleType.cpp
        src/event/market/Trade.cpp
        src/event/market/TradeBase.cpp
        src/event/market/TradeETH.cpp
        src/event/market/TradingStatus.cpp
)

set(dxFeedGraalCxxApi_EventOption_Sources
        src/event/option/Greeks.cpp
        src/event/option/Underlying.cpp
        src/event/option/TheoPrice.cpp
        src/event/option/Series.cpp
)

set(dxFeedGraalCxxApi_Schedule_Sources
        src/schedule/SessionFilter.cpp
        src/schedule/SessionType.cpp
)

set(dxFeedGraalCxxApi_Sources
        ${dxFeedGraalCxxApi_Internal_Sources}
        ${dxFeedGraalCxxApi_InternalUtils_Sources}
        ${dxFeedGraalCxxApi_InternalUtilsDebug_Sources}
        ${dxFeedGraalCxxApi_Api_Sources}
        ${dxFeedGraalCxxApi_ApiOsub_Sources}
        ${dxFeedGraalCxxApi_Ipf_Sources}
        ${dxFeedGraalCxxApi_Symbols_Sources}
        ${dxFeedGraalCxxApi_System_Sources}
        ${dxFeedGraalCxxApi_Event_Sources}
        ${dxFeedGraalCxxApi_EventCandle_Sources}
        ${dxFeedGraalCxxApi_EventMarket_Sources}
        ${dxFeedGraalCxxApi_EventOption_Sources}
        ${dxFeedGraalCxxApi_EventOption_Sources}
        ${dxFeedGraalCxxApi_Schedule_Sources}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path;@executable_path;@executable_path/../Frameworks")
elseif (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib64:$ORIGIN/../lib:$ORIGIN")
endif ()

add_library(${PROJECT_NAME}_static STATIC
        ${dxFeedGraalCxxApi_Sources}
        src/api.cpp
)

add_library(${PROJECT_NAME} SHARED
        ${dxFeedGraalCxxApi_Sources}
        src/api.cpp
)

add_library(dxfcxx ALIAS ${PROJECT_NAME})
add_library(dxfcxx::static ALIAS ${PROJECT_NAME}_static)
add_library(dxfcxx::graal ALIAS DxFeedGraalNativeSdk)

set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/range-v3-0.12/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS FMT_HEADER_ONLY=1)

target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_include_directories(${PROJECT_NAME}_static PRIVATE third_party/range-v3-0.12/include)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE
        FMT_HEADER_ONLY=1)

target_compile_options(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,MSVC>:
        /W2
        >
)

target_compile_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,MSVC>:
        /W2
        >
)

target_compile_options(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,GNU,Clang,AppleClang,Intel>:
        -fPIC
        -Wall
        -Wextra
        -pedantic
        >
)

target_link_options(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,GNU,Clang,AppleClang,Intel>:
        -fPIC
        >)

target_compile_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,GNU,Clang,AppleClang,Intel>:
        -fPIC
        -Wall
        -Wextra
        -pedantic
        >
)

target_link_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:C,CXX,GNU,Clang,AppleClang,Intel>:
        -fPIC
        >)

if (DXFCXX_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
    target_precompile_headers(${PROJECT_NAME}_static PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
endif ()

target_link_libraries(${PROJECT_NAME}_static PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date)
target_link_libraries(${PROJECT_NAME} PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date)

if (DXFCXX_ENABLE_ASAN_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_libraries(${PROJECT_NAME} PUBLIC asan ubsan)
endif ()

add_custom_command(TARGET ${PROJECT_NAME}_static POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}_static>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}>)

target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_VERSION="${DXFCXX_VERSION}")
target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_VERSION="${DXFCXX_VERSION}")

if (DXFCXX_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (DXFCXX_BUILD_SAMPLES)
    add_subdirectory(samples/cpp/PrintQuoteEvents)
    add_subdirectory(samples/cpp/DxFeedSample)
    add_subdirectory(samples/cpp/WriteTapeFile)
    add_subdirectory(samples/cpp/ConvertTapeFile)
    add_subdirectory(samples/cpp/DxFeedFileParser)
    add_subdirectory(samples/cpp/DxFeedConnect)
    add_subdirectory(samples/cpp/DxFeedIpfConnect)
    add_subdirectory(samples/cpp/DxFeedLiveIpfSample)
endif ()

if (DXFCXX_BUILD_TOOLS)
    add_subdirectory(tools/Tools)
endif ()

if (DXFCXX_INSTALL)
    include(GNUInstallDirs)
    set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}PackConfig.cmake")
    set(CPACK_SOURCE_GENERATOR ZIP)
    set(CPACK_PACKAGE_VENDOR "Devexperts LLC")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${DXFCXX_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${DXFCXX_VERSION}-${DXFCXX_TARGET_PLATFORM}${DXFCXX_PACKAGE_SUFFIX}")
    include(CPack)

    if (DXFCXX_INSTALL_LIB)
        #install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
        install(TARGETS ${PROJECT_NAME})
        install(TARGETS ${PROJECT_NAME}_static)
        install(DIRECTORY "include" DESTINATION "." PATTERN "build" EXCLUDE)
        install(FILES "LICENSE" "README.md" DESTINATION ".")
    endif ()
endif ()