# Copyright (c) 2025 Devexperts LLC.
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

set(DXFCXX_VERSION "v8.0.0" CACHE STRING "The dxFeed Graal CXX API package version")

dxfcxx_ParseVersion(${DXFCXX_VERSION} DXFCXX_MAJOR_VERSION DXFCXX_MINOR_VERSION DXFCXX_PATCH_VERSION DXFCXX_SUFFIX_VERSION)

set(DXFEED_GRAAL_NATIVE_SDK_VERSION "3.6.0" CACHE STRING "")
set(FMTLIB_VERSION "12.1.0")
set(BOOST_VERSION "1.84.0")
set(UTFCPP_VERSION "3.2.3")
set(DATE_VERSION "3.0.1")
set(PROCESS_VERSION "3.0.1")
set(CONSOLE_VERSION "1.0.1")
set(PORTALS_VERSION "0.1.1")
set(BITS_VERSION "1.0.0")
set(RANGE_VERSION "0.12")
set(DOCTEST_VERSION "2.4.11")
set(GLFW_VERSION "3.4")
set(IMGUI_VERSION "1.92.9b")
set(CPM_CMAKE_VERSION "v0.40.2")
set(CONFIG_VERSION "1.0.3")

# SHA-256 of the downloaded archives (URL_HASH): a changed or substituted archive fails the configuration.
set(FMTLIB_SHA256 "695fd197fa5aff8fc67b5f2bbc110490a875cdf7a41686ac8512fb480fa8ada7")
set(BOOST_SHA256 "2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e")
set(GLFW_SHA256 "a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a")
set(IMGUI_SHA256 "e1c46d676c2bcb7ced847ba27f50553e33a19db97b3cadaec7f8be64449139f8")
# The Graal Native SDK archives of the releases page by platform, for DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION only.
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION "3.6.0")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-linux "658c9521b31c3a478cc7c442a5b1f20baf5f6cd2065cd51bacb8f274773063b9")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-linux-debug "a5529ed84adeec3ad3d80a186c8c4809f95123e7dd3b5fb7f4471aedf7425e03")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-linux "9e2880a5620fcb8081f23e618bdd10ef94ae7766241bec2109e1f91f5be6c34a")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-linux-debug "c7b1db36bbd40f79d628ef0e8fbd948e33d476b61f1acae4c2fc88d6c74754af")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows "261d2a652f801d04fbcb7e1ff7765320deda15a00006e070e4dbcc4db79fb970")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-debug "3ec520f373d457a4eb3514fcccbf4bf426cec5af4b9c1cc637d9e4bbead75a46")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_x86_64-osx "ae39d1d01f3f258b79987bd8184eb9bd437dad21c26d9ca294a7ed0af7ea7a2f")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-osx "5c16d941260ef9959e7cbaad369389ba383411eeb5b5cd14126667d82c3360c4")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-ios "5a71228aaaade21ba7fb7dfdccc86425283fab0bf2c93e4c79a2bc422e4a7782")

include(cmake/third_party/CPM.cmake-${CPM_CMAKE_VERSION}/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT DEFINED DXFCXX_IS_ROOT_PROJECT)
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(DXFCXX_IS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else ()
        set(DXFCXX_IS_ROOT_PROJECT OFF)
    endif ()
endif ()

option(DXFCXX_BUILD_UNIT_TESTS "Build tests" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DXFCXX_BUILD_FUZZERS "Build the libFuzzer targets in fuzz/ (Clang only; instruments the static library for coverage)" OFF)
option(DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS "Dynamically link unit tests" OFF)
option(DXFCXX_ENABLE_NETWORK_TESTS "Build the unit tests that connect to demo.dxfeed.com (CTest label: network)" OFF)
option(DXFCXX_BUILD_SAMPLES "Build samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_DYNAMICALLY_LINK_SAMPLES "Dynamically link samples" OFF)
set(DXFCXX_BUILD_UI_SAMPLES "AUTO" CACHE STRING "Build the UI samples (Dear ImGui, GLFW, OpenGL): AUTO when their dependencies are found, ON (fail without them) or OFF")
set_property(CACHE DXFCXX_BUILD_UI_SAMPLES PROPERTY STRINGS AUTO ON OFF)
option(DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND "Require the GLFW Wayland backend of the UI samples (it is built whenever found)" OFF)
option(DXFCXX_BUILD_TOOLS "Build tools" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_DYNAMICALLY_LINK_TOOLS "Dynamically link tools" OFF)
option(DXFCXX_BUILD_DOC "Build documentation" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL "Prepare install" ON)
option(DXFCXX_INSTALL_LIB "Prepare install the libraries" ON)
option(DXFCXX_INSTALL_SAMPLES "Prepare install the samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL_TOOLS "Prepare install the tools" ${DXFCXX_IS_ROOT_PROJECT})

option(DXFCXX_LINK_STATIC_RUNTIME "Compile and link with -MT/-MTd or equivalent flag(s) to use a multi-threaded statically-linked runtime library. Visual Studio only." OFF)
option(DXFCXX_NODEFAULTLIB "Ignore libcmt/libcmtd/msvcrt/msvcrtd. Use if DXFCXX_LINK_STATIC_RUNTIME == ON." ${DXFCXX_LINK_STATIC_RUNTIME})

option(DXFCXX_ENABLE_METRICS "Enable metrics collection" OFF)

option(DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_GITHUB "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_BASE_URL "https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/download/" CACHE STRING "")
option(DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK "" OFF)

option(DXFCXX_USE_DXFG_STUB "Use dxFeed Graal SDK stub" OFF)
set(DXFG_STUB_REPO "https://github.com/ttldtor/DxFeedGraalNativeSdkStub" CACHE STRING "")

set(DXFG_LOCAL_ARCHIVE_PATH "" CACHE STRING "")

option(DXFCXX_ENABLE_ASAN "Enable AddressSanitizer for the library and all its consumers" OFF)
option(DXFCXX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (can be combined with ASan; not supported by MSVC)" OFF)
option(DXFCXX_ENABLE_TSAN "Enable ThreadSanitizer (cannot be combined with ASan; not supported by MSVC)" OFF)
option(DXFCXX_USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)
option(DXFCXX_BUILD_HEADER_CHECK "Compile every public header on its own to check that it is self-contained" OFF)

option(DXFCXX_FEATURE_STACKTRACE "Allow capturing stacktraces using boost::stacktrace" OFF)

# https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
# https://github.com/microsoft/STL/wiki/VS-2022-Changelog#vs-2022-1710
# https://github.com/microsoft/STL/pull/3824
# https://github.com/actions/runner-images/issues/10004
option(DXFCXX_MSVC_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR "Enable _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR workaround for old MSVC runtimes" OFF)


option(DXFCXX_TRACING "Enable tracing" OFF)
option(DXFCXX_LIST_TRACING "Enable list tracing" OFF)
option(DXFCXX_ISOLATES_TRACING "Enable isolates tracing" OFF)

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
include(cmake/Sanitizers.cmake)
include(cmake/SetupStaticRuntimeMSVC.cmake)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-${DXFCXX_GRAAL_TARGET_PLATFORM}/CMakeLists.txt")
    add_subdirectory(third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-${DXFCXX_GRAAL_TARGET_PLATFORM})
else ()
    if (NOT DXFG_LOCAL_ARCHIVE_PATH STREQUAL "")
        FetchContent_Declare(DxFeedGraalNativeSdk
                URL "${DXFG_LOCAL_ARCHIVE_PATH}"
        )
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
                if (DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_GITHUB)
                    set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_BASE_URL}v${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
                endif ()

                # The platforms that have Graal Native SDK archives on the releases page.
                set(DXFCXX_GRAAL_SUPPORTED_PLATFORMS amd64-linux aarch64-linux amd64-windows x86_64-osx aarch64-osx aarch64-ios)
                set(DXFCXX_GRAAL_SUPPORTED_DEBUG_PLATFORMS amd64-linux aarch64-linux amd64-windows)

                if (DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK)
                    set(DXFCXX_GRAAL_SUPPORTED_PLATFORMS ${DXFCXX_GRAAL_SUPPORTED_DEBUG_PLATFORMS})
                endif ()

                if (NOT DXFCXX_GRAAL_TARGET_PLATFORM IN_LIST DXFCXX_GRAAL_SUPPORTED_PLATFORMS)
                    list(JOIN DXFCXX_GRAAL_SUPPORTED_PLATFORMS ", " DXFCXX_GRAAL_SUPPORTED_PLATFORMS_TEXT)
                    message(FATAL_ERROR "Unsupported platform ${DXFCXX_GRAAL_TARGET_PLATFORM}: the Graal Native SDK archives are built for ${DXFCXX_GRAAL_SUPPORTED_PLATFORMS_TEXT}. To use another build, set DXFG_LOCAL_ARCHIVE_PATH or the DXFEED_GRAAL_NATIVE_SDK_URL environment variable.")
                endif ()

                set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE ${DXFCXX_GRAAL_TARGET_PLATFORM})

                if (DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK)
                    set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}-debug)
                endif ()

                set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}.zip)
                set(DXFEED_GRAAL_NATIVE_SDK_URL_HASH "")

                if (DXFEED_GRAAL_NATIVE_SDK_VERSION STREQUAL DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION)
                    set(DXFEED_GRAAL_NATIVE_SDK_URL_HASH URL_HASH SHA256=${DXFEED_GRAAL_NATIVE_SDK_SHA256_${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}})
                else ()
                    message(WARNING "No SHA-256 for the Graal Native SDK ${DXFEED_GRAAL_NATIVE_SDK_VERSION} (known: ${DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION}): the downloaded archive is not verified")
                endif ()

                FetchContent_Declare(
                        DxFeedGraalNativeSdk
                        URL ${DXFEED_GRAAL_NATIVE_SDK_URL}
                        ${DXFEED_GRAAL_NATIVE_SDK_URL_HASH}
                        USES_TERMINAL_DOWNLOAD TRUE
                        GIT_PROGRESS TRUE
                        DOWNLOAD_NO_EXTRACT FALSE
                )
            endif ()
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

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(process ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Console-${CONSOLE_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/Console-${CONSOLE_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/Console#v${CONSOLE_VERSION}")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(console ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

# bits (header-only): the bit operations of the library headers. It is declared before portals, which fetches it with
# the same name: the first declaration is used. EXCLUDE_FROM_ALL (CMake 3.28+) disables its own install rules; its
# headers are installed with the library below.
if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_BITS AND
        EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bits-${BITS_VERSION}/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_BITS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bits-${BITS_VERSION}")
endif ()

FetchContent_Declare(
        bits
        GIT_REPOSITORY https://github.com/ttldtor/bits.git
        GIT_TAG 9b59e45d2cc75303ffc4cc6d5b42363f8ab3508b # v1.0.0
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(bits)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/portals-${PORTALS_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/portals-${PORTALS_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/portals#v${PORTALS_VERSION}")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(portals ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

add_subdirectory(third_party/utfcpp-${UTFCPP_VERSION})

set(FMT_INSTALL OFF)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt-${FMTLIB_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/fmt-${FMTLIB_VERSION})
else ()
    FetchContent_Declare(
            fmt
            URL https://github.com/fmtlib/fmt/releases/download/${FMTLIB_VERSION}/fmt-${FMTLIB_VERSION}.zip # downloading a zip release speeds up the download
            URL_HASH SHA256=${FMTLIB_SHA256}
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            DOWNLOAD_NO_EXTRACT FALSE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(fmt)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(fmt ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

#set(BUILD_TZ_LIB ON)
#set(USE_SYSTEM_TZ_DB ON)
add_subdirectory(third_party/date-${DATE_VERSION})

if (DXFCXX_FEATURE_STACKTRACE)
    set(BOOST_INCLUDE_LIBRARIES stacktrace)

    if (DXFCXX_LINK_STATIC_RUNTIME AND
            (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
                    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")))
        set(BOOST_RUNTIME_LINK static CACHE STRING "Boost runtime library selection for the MS ABI" FORCE)
    endif ()

    CPMAddPackage(
            NAME Boost
            VERSION ${BOOST_VERSION}
            URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}.tar.xz
            URL_HASH SHA256=${BOOST_SHA256}
            OPTIONS "BOOST_ENABLE_CMAKE ON"
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
endif ()

# config (header-only) is used by the library sources only: EXCLUDE_FROM_ALL keeps its install rules (headers and
# a CMake package config) out of the packages.
add_subdirectory(third_party/config-${CONFIG_VERSION} EXCLUDE_FROM_ALL)

set(dxFeedGraalCxxApi_Internal_Sources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/JavaObjectHandle.cpp
        src/internal/EventClassList.cpp
        src/internal/Common.cpp
        src/internal/Metrics.cpp
        src/internal/Platform.cpp
        src/internal/StopWatch.cpp
        src/internal/TimeFormat.cpp
        src/internal/Timer.cpp
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

set(dxFeedGraalCxxApi_Glossary_Sources
        src/glossary/AdditionalUnderlyings.cpp
        src/glossary/CFI.cpp
        src/glossary/PriceIncrements.cpp
)

set(dxFeedGraalCxxApi_Isolated_Sources
        src/isolated/api/IsolatedDXFeed.cpp
        src/isolated/api/IsolatedDXEndpoint.cpp
        src/isolated/api/IsolatedDXFeedSubscription.cpp
        src/isolated/api/IsolatedDXPublisher.cpp
        src/isolated/api/IsolatedDXPublisherObservableSubscription.cpp
        src/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.cpp
        src/isolated/candlewebservice/IsolatedHistoryEndpoint.cpp
        src/isolated/auth/IsolatedAuthToken.cpp
        src/isolated/event/IsolatedEventType.cpp
        src/isolated/executors/IsolatedInPlaceExecutor.cpp
        src/isolated/glossary/IsolatedAdditionalUnderlyings.cpp
        src/isolated/glossary/IsolatedCFI.cpp
        src/isolated/glossary/IsolatedPriceIncrements.cpp
        src/isolated/internal/IsolatedString.cpp
        src/isolated/internal/IsolatedObject.cpp
        src/isolated/internal/IsolatedTimeFormat.cpp
        src/isolated/internal/IsolatedTools.cpp
        src/isolated/promise/IsolatedPromise.cpp
        src/isolated/ipf/IsolatedInstrumentProfile.cpp
        src/isolated/ipf/IsolatedInstrumentProfileReader.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileCollector.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileConnection.cpp
        src/isolated/logging/IsolatedLogging.cpp
        src/isolated/metrics/IsolatedMetrics.cpp
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

set(dxFeedGraalCxxApi_CandleWebService_Sources
        src/candlewebservice/HistoryEndpoint.cpp
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

set(dxFeedGraalCxxApi_Logging_Sources
        src/logging/Logging.cpp
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
        src/event/TimeSeriesEvent.cpp
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
        src/event/market/MarketEventSymbols.cpp
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
        src/event/misc/TextMessage.cpp
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
        ${dxFeedGraalCxxApi_Glossary_Sources}
        ${dxFeedGraalCxxApi_Api_Sources}
        ${dxFeedGraalCxxApi_ApiOsub_Sources}
        ${dxFeedGraalCxxApi_Auth_Sources}
        ${dxFeedGraalCxxApi_CandleWebService_Sources}
        ${dxFeedGraalCxxApi_Ipf_Sources}
        ${dxFeedGraalCxxApi_Logging_Sources}
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
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path/../../../${CMAKE_INSTALL_LIBDIR};@loader_path;@executable_path;@executable_path/../Frameworks")
elseif (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../../../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib64:$ORIGIN/../lib:$ORIGIN")
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

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    # set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    SetupStaticRuntimeMSVC(${PROJECT_NAME}_static ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_include_directories(${PROJECT_NAME}_static PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE
        FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

# fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
set(DXFCXX_MSVC_COMPILE_OPTIONS /W4 /bigobj /utf-8)
set(DXFCXX_MSVC_DEBUG_LINK_OPTIONS /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>)
set(DXFCXX_GCC_LIKE_COMPILE_OPTIONS -fPIC -Wall -Wextra -pedantic)
set(DXFCXX_GCC_LIKE_LINK_OPTIONS "-fPIC")

#A workaround to fix 'relocation truncated to fit: IMAGE_REL_AMD64_SECREL' error (MinGW + Debug)
set(DXFCXX_MINGW_DEBUG_LINK_OPTIONS "-Wl,--disable-dynamicbase,--disable-high-entropy-va,--default-image-base-low")

add_library(dxfcxx_mingw_debug_link_options_interface INTERFACE)

if (MINGW)
    target_link_options(dxfcxx_mingw_debug_link_options_interface
            INTERFACE
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${DXFCXX_MINGW_DEBUG_LINK_OPTIONS}>
    )
endif ()

if (DXFCXX_MSVC_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    # https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
    # https://github.com/microsoft/STL/wiki/VS-2022-Changelog#vs-2022-1710
    # https://github.com/microsoft/STL/pull/3824
    # https://github.com/actions/runner-images/issues/10004
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_definitions(${PROJECT_NAME} PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    endif ()
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(DXFCXX_GCC_LIKE_COMPILE_OPTIONS ${DXFCXX_GCC_LIKE_COMPILE_OPTIONS} "-Wa,--noexecstack")
    set(DXFCXX_GCC_LIKE_LINK_OPTIONS ${DXFCXX_GCC_LIKE_LINK_OPTIONS} "-Wl,-z,noexecstack")
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

if (DXFCXX_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
    target_precompile_headers(${PROJECT_NAME}_static PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
endif ()

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        bits::bits
        PRIVATE
        DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config dxfcxx_mingw_debug_link_options_interface
)

target_link_libraries(${PROJECT_NAME}_static
        PUBLIC
        bits::bits
        PRIVATE
        DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config dxfcxx_mingw_debug_link_options_interface
)

if (DXFCXX_FEATURE_STACKTRACE)
    LinkStacktrace(${PROJECT_NAME})
    LinkStacktrace(${PROJECT_NAME}_static)

    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_FEATURE_STACKTRACE)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_FEATURE_STACKTRACE)
endif ()

# Both library targets are instrumented; PUBLIC propagates the options and the sanitizer runtime to every consumer.
target_link_libraries(${PROJECT_NAME} PUBLIC dxfcxx_sanitizers)
target_link_libraries(${PROJECT_NAME}_static PUBLIC dxfcxx_sanitizers)

if (DXFCXX_BUILD_FUZZERS)
    # libFuzzer ships with LLVM Clang only (not with AppleClang, GCC or MSVC).
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "DXFCXX_BUILD_FUZZERS requires LLVM Clang (libFuzzer), the compiler is ${CMAKE_CXX_COMPILER_ID}")
    endif ()

    # Coverage instrumentation of the library code for libFuzzer; the fuzz targets link libFuzzer itself.
    target_compile_options(${PROJECT_NAME}_static PRIVATE -fsanitize=fuzzer-no-link)
endif ()

if (DXFCXX_SANITIZERS_ENABLED)
    # Deliberately defective functions for the sanitizer canary tests (tests/sanitizers); sanitized builds only.
    target_sources(${PROJECT_NAME} PRIVATE src/internal/SanitizerCanary.cpp)
    target_sources(${PROJECT_NAME}_static PRIVATE src/internal/SanitizerCanary.cpp)
endif ()

if (DXFCXX_BUILD_HEADER_CHECK)
    # One translation unit per public header, containing only `#include <header>` and compiled without precompiled
    # headers: a header that relies on includes it does not make fails to compile here.
    file(GLOB_RECURSE DXFCXX_PUBLIC_HEADERS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
    set(DXFCXX_HEADER_CHECK_SOURCES "")

    foreach (header ${DXFCXX_PUBLIC_HEADERS})
        string(MAKE_C_IDENTIFIER "${header}" headerId)

        if (header MATCHES "\\.h$")
            set(source "${CMAKE_CURRENT_BINARY_DIR}/header_check/${headerId}.c") # C headers are checked as C
        else ()
            set(source "${CMAKE_CURRENT_BINARY_DIR}/header_check/${headerId}.cpp")
        endif ()

        file(CONFIGURE OUTPUT "${source}" CONTENT "#include <${header}>\n")
        list(APPEND DXFCXX_HEADER_CHECK_SOURCES "${source}")
    endforeach ()

    add_library(dxfcxx_header_check OBJECT ${DXFCXX_HEADER_CHECK_SOURCES})
    # The same include directories, definitions, options and dependencies as the library itself.
    target_link_libraries(dxfcxx_header_check PRIVATE ${PROJECT_NAME}_static
            DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config)
    target_include_directories(dxfcxx_header_check PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
    target_compile_definitions(dxfcxx_header_check PRIVATE FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)
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

if (DXFCXX_ENABLE_METRICS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_ENABLE_METRICS=1)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_ENABLE_METRICS=1)
endif ()

if (DXFCXX_TRACING)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_DEBUG=1)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_DEBUG=1)

    if (DXFCXX_LIST_TRACING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_TRACE_LISTS=1)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_TRACE_LISTS=1)
    endif ()

    if (DXFCXX_ISOLATES_TRACING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_TRACE_ISOLATES=1)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_TRACE_ISOLATES=1)
    endif ()
endif ()

if (DXFCXX_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (DXFCXX_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif ()

if (DXFCXX_BUILD_SAMPLES)
    add_subdirectory(samples/cpp/API/AuthSample)
    add_subdirectory(samples/cpp/API/ConnectSample)
    add_subdirectory(samples/cpp/API/PrintQuoteEventsSample)
    add_subdirectory(samples/cpp/API/PublishProfilesSample)
    add_subdirectory(samples/cpp/API/QuoteAndTradeSample)
    add_subdirectory(samples/cpp/API/ReconnectSample)
    add_subdirectory(samples/cpp/API/RequestProfileSample)
    add_subdirectory(samples/cpp/Candle/FetchDailyCandlesSample)
    add_subdirectory(samples/cpp/Candle/CandleDataSample)
    add_subdirectory(samples/cpp/Console/LastEventConsoleSample)
    add_subdirectory(samples/cpp/File/ConvertTapeFileSample)
    add_subdirectory(samples/cpp/File/FileParserSample)
    add_subdirectory(samples/cpp/File/WriteTapeFileSample)
    add_subdirectory(samples/cpp/IPF/IpfSample)
    add_subdirectory(samples/cpp/IPF/IpfLiveSample)
    add_subdirectory(samples/cpp/IPF/OptionChainSample)
    add_subdirectory(samples/cpp/Model/IncOrderSnapshotSample)
    add_subdirectory(samples/cpp/Model/MarketDepthModelSample)
    add_subdirectory(samples/cpp/Model/MultipleMarketDepthSample)
    add_subdirectory(samples/cpp/Model/PriceLevelBookSample)
    add_subdirectory(samples/cpp/OnDemand/OnDemandSample)
    add_subdirectory(samples/cpp/Schedule/ScheduleSample)

    include(cmake/UiSamples.cmake)
    dxfcxx_check_ui_samples(DXFCXX_UI_SAMPLES_ENABLED)

    if (DXFCXX_UI_SAMPLES_ENABLED)
        add_subdirectory(samples/cpp/UI/DXFeedTimeAndSalesSample)
    endif ()
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
        # The public headers include <bits/bits.hpp>.
        install(DIRECTORY "${bits_SOURCE_DIR}/include/" DESTINATION "include")
    endif ()

    install(FILES "DEPENDENCIES.md" "LICENSE" "README.md" "ReleaseNotes.md" "THIRD_PARTY_LICENSES.md" DESTINATION ".")
endif ()
