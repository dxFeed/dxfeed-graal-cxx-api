# Copyright (c) 2024 Devexperts LLC.
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

include(cmake/ParseVersion.cmake)

set(DXFCXX_VERSION "v3.0.0" CACHE STRING "The dxFeed Graal CXX API package version")

dxfcxx_ParseVersion(${DXFCXX_VERSION} DXFCXX_MAJOR_VERSION DXFCXX_MINOR_VERSION DXFCXX_PATCH_VERSION DXFCXX_SUFFIX_VERSION)

set(DXFEED_GRAAL_NATIVE_SDK_VERSION "2.1.1" CACHE STRING "")
set(FMTLIB_VERSION "11.0.2")
set(BOOST_VERSION "1.84.0")
set(UTFCPP_VERSION "3.2.3")
set(DATE_VERSION "3.0.1")
set(PROCESS_VERSION "3.0.1")
set(CONSOLE_VERSION "1.0.1")
set(RANGE_VERSION "0.12")
set(DOCTEST_VERSION "2.4.11")
set(CPM_CMAKE_VERSION "v0.40.2")

include(cmake/third_party/CPM.cmake-${CPM_CMAKE_VERSION}/CPM.cmake)

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
option(DXFCXX_BUILD_DOC "Build documentation" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL "Prepare install" ON)
option(DXFCXX_INSTALL_LIB "Prepare install the libraries" ON)
option(DXFCXX_INSTALL_SAMPLES "Prepare install the samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL_TOOLS "Prepare install the tools" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/" CACHE STRING "")

option(DXFCXX_USE_DXFG_STUB "Use dxFeed Graal SDK stub" OFF)
set(DXFG_STUB_REPO "https://github.com/ttldtor/DxFeedGraalNativeSdkStub" CACHE STRING "")

option(DXFCXX_ENABLE_ASAN_UBSAN "Enable address, UB sanitizers etc" OFF)
option(DXFCXX_ENABLE_VS_ASAN "Enable VS address sanitizer" OFF)
option(DXFCXX_USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)

option(DXFCXX_FEATURE_STACKTRACE "Allow capturing stacktraces using boost::stacktrace" OFF)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

set(DXFCXX_GRAAL_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_TARGET_CPU "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_CPU "unknown" CACHE STRING "")

include(cmake/ParseAndDetectPlatforms.cmake)
include(cmake/LinkStacktrace.cmake)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-${DXFCXX_GRAAL_TARGET_PLATFORM}/CMakeLists.txt")
    add_subdirectory(third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-${DXFCXX_GRAAL_TARGET_PLATFORM})
else ()
    if (DXFCXX_USE_DXFG_STUB)
        FetchContent_Declare(
                DxFeedGraalNativeSdk
                GIT_REPOSITORY "${DXFG_STUB_REPO}.git" GIT_TAG "v${DXFEED_GRAAL_NATIVE_SDK_VERSION}"
                USES_TERMINAL_DOWNLOAD TRUE
                GIT_PROGRESS TRUE
                DOWNLOAD_NO_EXTRACT FALSE
        )
    else ()
        if (DEFINED ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
            FetchContent_Declare(
                    DxFeedGraalNativeSdk
                    URL $ENV{DXFEED_GRAAL_NATIVE_SDK_URL}
                    USES_TERMINAL_DOWNLOAD TRUE
                    GIT_PROGRESS TRUE
                    DOWNLOAD_NO_EXTRACT FALSE
            )
        else ()
            if (DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_JFROG)
                set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_JFROG_BASE_URL}${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
            endif ()

            if (DXFCXX_GRAAL_TARGET_PLATFORM STREQUAL "unknown-unknown")
                message(ERROR "Unsupported platform!")
            else ()
                set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-${DXFCXX_GRAAL_TARGET_PLATFORM}.zip)
            endif ()

            FetchContent_Declare(
                    DxFeedGraalNativeSdk
                    URL ${DXFEED_GRAAL_NATIVE_SDK_URL}
                    USES_TERMINAL_DOWNLOAD TRUE
                    GIT_PROGRESS TRUE
                    DOWNLOAD_NO_EXTRACT FALSE
            )
        endif ()
    endif ()

    FetchContent_MakeAvailable(DxFeedGraalNativeSdk)
    # DxFeedGraalNativeSdk_SOURCE_DIR
endif ()


if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Process-${PROCESS_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/Process-${PROCESS_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/Process#v${PROCESS_VERSION}")
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Console-${CONSOLE_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/Console-${CONSOLE_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/Console#v${CONSOLE_VERSION}")
endif ()

add_subdirectory(third_party/utfcpp-${UTFCPP_VERSION})

set(FMT_INSTALL OFF)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt-${FMTLIB_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/fmt-${FMTLIB_VERSION})
else ()
    FetchContent_Declare(
            fmt
            URL https://github.com/fmtlib/fmt/releases/download/${FMTLIB_VERSION}/fmt-${FMTLIB_VERSION}.zip # downloading a zip release speeds up the download
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            DOWNLOAD_NO_EXTRACT FALSE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(fmt)
endif ()

#set(BUILD_TZ_LIB ON)
#set(USE_SYSTEM_TZ_DB ON)
add_subdirectory(third_party/date-${DATE_VERSION})

if (DXFCXX_FEATURE_STACKTRACE)
    set(BOOST_INCLUDE_LIBRARIES stacktrace)

    CPMAddPackage(
            NAME Boost
            VERSION ${BOOST_VERSION}
            URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}.tar.xz
            OPTIONS "BOOST_ENABLE_CMAKE ON"
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
endif ()

set(dxFeedGraalCxxApi_Internal_Sources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/JavaObjectHandle.cpp
        src/internal/EventClassList.cpp
        src/internal/Common.cpp
        src/internal/Platform.cpp
        src/internal/TimeFormat.cpp
)

set(dxFeedGraalCxxApi_InternalUtils_Sources
        src/internal/utils/StringUtils.cpp
        src/internal/utils/EnumUtils.cpp
        src/internal/utils/CmdArgsUtils.cpp
)

set(dxFeedGraalCxxApi_InternalUtilsDebug_Sources
        src/internal/utils/debug/Debug.cpp
)

set(dxFeedGraalCxxApi_Exceptions_Sources
        src/exceptions/RuntimeException.cpp
        src/exceptions/InvalidArgumentException.cpp
        src/exceptions/GraalException.cpp
        src/exceptions/JavaException.cpp
)

set(dxFeedGraalCxxApi_Executors_Sources
        src/executors/InPlaceExecutor.cpp
)

set(dxFeedGraalCxxApi_Isolated_Sources
        src/isolated/api/IsolatedDXFeed.cpp
        src/isolated/api/IsolatedDXEndpoint.cpp
        src/isolated/api/IsolatedDXFeedSubscription.cpp
        src/isolated/api/IsolatedDXPublisher.cpp
        src/isolated/api/IsolatedDXPublisherObservableSubscription.cpp
        src/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.cpp
        src/isolated/auth/IsolatedAuthToken.cpp
        src/isolated/event/IsolatedEventType.cpp
        src/isolated/executors/IsolatedInPlaceExecutor.cpp
        src/isolated/internal/IsolatedString.cpp
        src/isolated/internal/IsolatedObject.cpp
        src/isolated/internal/IsolatedTimeFormat.cpp
        src/isolated/internal/IsolatedTools.cpp
        src/isolated/promise/IsolatedPromise.cpp
        src/isolated/ipf/IsolatedInstrumentProfile.cpp
        src/isolated/ipf/IsolatedInstrumentProfileReader.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileCollector.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileConnection.cpp
        src/isolated/model/IsolatedTxModelListener.cpp
        src/isolated/model/IsolatedIndexedTxModel.cpp
        src/isolated/model/IsolatedTimeSeriesTxModel.cpp
        src/isolated/ondemand/IsolatedOnDemandService.cpp
        src/isolated/util/IsolatedTimePeriod.cpp
        src/isolated/schedule/IsolatedDay.cpp
        src/isolated/schedule/IsolatedSession.cpp
        src/isolated/schedule/IsolatedSchedule.cpp
)

set(dxFeedGraalCxxApi_Api_Sources
        src/api/DXEndpoint.cpp
        src/api/DXFeed.cpp
        src/api/DXFeedSubscription.cpp
        src/api/DXPublisherObservableSubscription.cpp
        src/api/DXPublisher.cpp
)

set(dxFeedGraalCxxApi_ApiOsub_Sources
        src/api/osub/WildcardSymbol.cpp
        src/api/osub/TimeSeriesSubscriptionSymbol.cpp
        src/api/osub/IndexedEventSubscriptionSymbol.cpp
        src/api/osub/ObservableSubscriptionChangeListener.cpp
)

set(dxFeedGraalCxxApi_Auth_Sources
        src/auth/AuthToken.cpp
)

set(dxFeedGraalCxxApi_Ipf_Sources
        src/ipf/InstrumentProfile.cpp
        src/ipf/InstrumentProfileType.cpp
        src/ipf/InstrumentProfileField.cpp
        src/ipf/InstrumentProfileReader.cpp
        src/ipf/live/InstrumentProfileCollector.cpp
        src/ipf/live/InstrumentProfileConnection.cpp
        src/ipf/live/IterableInstrumentProfile.cpp
)

set(dxFeedGraalCxxApi_Model_Sources
        src/model/TxModelListener.cpp
        src/model/IndexedTxModel.cpp
        src/model/TimeSeriesTxModel.cpp
        src/model/MarketDepthModel.cpp
)

set(dxFeedGraalCxxApi_OnDemand_Sources
        src/ondemand/OnDemandService.cpp
)

set(dxFeedGraalCxxApi_Promise_Sources
        src/promise/Promise.cpp
        src/promise/Promises.cpp
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
        src/event/EventSourceWrapper.cpp
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
        src/event/market/OtcMarketsPriceType.cpp
        src/event/market/OtcMarketsOrder.cpp
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

set(dxFeedGraalCxxApi_EventMisc_Sources
        src/event/misc/Message.cpp
)

set(dxFeedGraalCxxApi_Schedule_Sources
        src/schedule/SessionFilter.cpp
        src/schedule/SessionType.cpp
        src/schedule/DayFilter.cpp
        src/schedule/Day.cpp
        src/schedule/Session.cpp
        src/schedule/Schedule.cpp
)

set(dxFeedGraalCxxApi_Util_Sources
        src/util/TimePeriod.cpp
)

set(dxFeedGraalCxxApi_Sources
        ${dxFeedGraalCxxApi_Internal_Sources}
        ${dxFeedGraalCxxApi_InternalUtils_Sources}
        ${dxFeedGraalCxxApi_InternalUtilsDebug_Sources}
        ${dxFeedGraalCxxApi_Isolated_Sources}
        ${dxFeedGraalCxxApi_Exceptions_Sources}
        ${dxFeedGraalCxxApi_Executors_Sources}
        ${dxFeedGraalCxxApi_Api_Sources}
        ${dxFeedGraalCxxApi_ApiOsub_Sources}
        ${dxFeedGraalCxxApi_Auth_Sources}
        ${dxFeedGraalCxxApi_Ipf_Sources}
        ${dxFeedGraalCxxApi_Model_Sources}
        ${dxFeedGraalCxxApi_OnDemand_Sources}
        ${dxFeedGraalCxxApi_Promise_Sources}
        ${dxFeedGraalCxxApi_Symbols_Sources}
        ${dxFeedGraalCxxApi_System_Sources}
        ${dxFeedGraalCxxApi_Event_Sources}
        ${dxFeedGraalCxxApi_EventCandle_Sources}
        ${dxFeedGraalCxxApi_EventMarket_Sources}
        ${dxFeedGraalCxxApi_EventOption_Sources}
        ${dxFeedGraalCxxApi_EventMisc_Sources}
        ${dxFeedGraalCxxApi_Schedule_Sources}
        ${dxFeedGraalCxxApi_Util_Sources}
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

if (WIN32)
    if (MINGW)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            list(APPEND CMAKE_RC_FLAGS "--use-temp-file")
        endif ()
    endif ()

    add_library(${PROJECT_NAME} SHARED
            ${dxFeedGraalCxxApi_Sources}
            resources/dxFeedGraalCxxApi.rc
            src/api.cpp
    )
else ()
    add_library(${PROJECT_NAME} SHARED
            ${dxFeedGraalCxxApi_Sources}
            src/api.cpp
    )
endif ()

add_library(dxfcxx ALIAS ${PROJECT_NAME})
add_library(dxfcxx::static ALIAS ${PROJECT_NAME}_static)
add_library(dxfcxx::graal ALIAS DxFeedGraalNativeSdk)

set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_include_directories(${PROJECT_NAME}_static PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE
        FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

set(DXFCXX_MSVC_COMPILE_OPTIONS "/W4")
set(DXFCXX_MSVC_DEBUG_LINK_OPTIONS "/PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>")
set(DXFCXX_GCC_LIKE_COMPILE_OPTIONS -fPIC -Wall -Wextra -pedantic)
set(DXFCXX_GCC_LIKE_LINK_OPTIONS "-fPIC")

#A workaround to fix 'relocation truncated to fit: IMAGE_REL_AMD64_SECREL' error (MinGW + Debug)
set(DXFCXX_MINGW_DEBUG_LINK_OPTIONS "-Wl,--disable-dynamicbase,--disable-high-entropy-va,--default-image-base-low")

# https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
# https://github.com/microsoft/STL/wiki/Changelog#vs-2022-1710
# https://github.com/actions/runner-images/issues/10004
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${PROJECT_NAME} PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif ()

target_compile_options(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
)

target_link_options(${PROJECT_NAME}
        PUBLIC
        $<$<AND:$<COMPILE_LANG_AND_ID:CXX,MSVC>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:${DXFCXX_MSVC_DEBUG_LINK_OPTIONS}>
        $<$<AND:$<COMPILE_LANG_AND_ID:C,MSVC>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:${DXFCXX_MSVC_DEBUG_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
)

if (MINGW)
    target_link_options(${PROJECT_NAME}
            PUBLIC
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${DXFCXX_MINGW_DEBUG_LINK_OPTIONS}>
    )
endif ()

target_compile_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
)

target_link_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
)

if (MINGW)
    target_link_options(${PROJECT_NAME}_static
            PUBLIC
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${DXFCXX_MINGW_DEBUG_LINK_OPTIONS}>
    )
endif ()

if (DXFCXX_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
    target_precompile_headers(${PROJECT_NAME}_static PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
endif ()

target_link_libraries(${PROJECT_NAME} PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date)
target_link_libraries(${PROJECT_NAME}_static PUBLIC DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date)

if (DXFCXX_FEATURE_STACKTRACE)
    LinkStacktrace(${PROJECT_NAME})
    LinkStacktrace(${PROJECT_NAME}_static)

    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_FEATURE_STACKTRACE)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_FEATURE_STACKTRACE)
endif ()

if (DXFCXX_ENABLE_ASAN_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_libraries(${PROJECT_NAME} PUBLIC asan ubsan)
endif ()

add_custom_command(TARGET ${PROJECT_NAME}_static POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}_static>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DxFeedGraalNativeSdk> $<TARGET_FILE_DIR:${PROJECT_NAME}>)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        DXFCXX_VERSION="${DXFCXX_VERSION}"
        DXFCXX_MAJOR_VERSION=${DXFCXX_MAJOR_VERSION}
        DXFCXX_MINOR_VERSION=${DXFCXX_MINOR_VERSION}
        DXFCXX_PATCH_VERSION=${DXFCXX_PATCH_VERSION}
)

target_compile_definitions(${PROJECT_NAME}_static PUBLIC
        DXFCXX_VERSION="${DXFCXX_VERSION}"
        DXFCXX_MAJOR_VERSION=${DXFCXX_MAJOR_VERSION}
        DXFCXX_MINOR_VERSION=${DXFCXX_MINOR_VERSION}
        DXFCXX_PATCH_VERSION=${DXFCXX_PATCH_VERSION}
)

if (DXFCXX_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (DXFCXX_BUILD_SAMPLES)
    add_subdirectory(samples/cpp/PrintQuoteEvents)
    add_subdirectory(samples/cpp/PublishProfiles)
    add_subdirectory(samples/cpp/DxFeedSample)
    add_subdirectory(samples/cpp/FetchDailyCandles)
    add_subdirectory(samples/cpp/WriteTapeFile)
    add_subdirectory(samples/cpp/ConvertTapeFile)
    add_subdirectory(samples/cpp/DxFeedFileParser)
    add_subdirectory(samples/cpp/DxFeedConnect)
    add_subdirectory(samples/cpp/DxFeedIpfConnect)
    add_subdirectory(samples/cpp/DxFeedLiveIpfSample)
    add_subdirectory(samples/cpp/ScheduleSample)
    add_subdirectory(samples/cpp/OnDemandSample)
endif ()

add_subdirectory(docs)

if (DXFCXX_BUILD_TOOLS)
    add_subdirectory(tools/Tools)
endif ()

if (DXFCXX_INSTALL)
    include(GNUInstallDirs)
    set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}PackConfig.cmake")
    set(CPACK_SOURCE_GENERATOR ZIP)
    set(CPACK_PACKAGE_VENDOR "Devexperts LLC")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${DXFCXX_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${DXFCXX_VERSION}-${DXFCXX_TARGET_PLATFORM}${DXFCXX_PACKAGE_SUFFIX}")
    include(CPack)

    if (DXFCXX_INSTALL_LIB)
        #install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
        install(TARGETS ${PROJECT_NAME})
        install(TARGETS ${PROJECT_NAME}_static)
        install(DIRECTORY "include" DESTINATION "." PATTERN "build" EXCLUDE)
    endif ()

    install(FILES "DEPENDENCIES.md" "LICENSE" "README.md" "ReleaseNotes.md" "THIRD_PARTY_LICENSES.md" DESTINATION ".")
endif ()