# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.15)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(dxFeedGraalCXXAPITests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(DXFC_TEST_INCLUDE_DIRS ../include ../third_party/doctest-${DOCTEST_VERSION} ../third_party/range-v3-${RANGE_VERSION}/include)

if (DXFCXX_BUILD_BENCHMARKS)
    FetchContent_Declare(
            nanobench
            GIT_REPOSITORY https://github.com/martinus/nanobench.git
            GIT_TAG a5a50c2b33eea2ff1fcb355cacdface43eb42b25 # v4.3.11
    )
    FetchContent_MakeAvailable(nanobench)
endif ()

set(DXFC_TEST_SOURCES
        api/CandleSymbolTest.cpp
        api/DataIntegrityTest.cpp
        api/DXFeedTest.cpp
        api/DXEndpointTest.cpp
        api/DXPublisherTest.cpp
        api/EventsTest.cpp
        api/MarketEventSymbolsTest.cpp
        api/ObservableSubscriptionLifecycleTest.cpp
        api/OrderSourceTest.cpp
        event/EdgeValuesTapeTest.cpp
        event/EventsTest.cpp
        event/TapeRoundTripTest.cpp
        exceptions/ExceptionsTest.cpp
        glossary/AdditionalUnderlyingsTest.cpp
        glossary/PriceIncrementsTest.cpp
        internal/CommonTest.cpp
        internal/EntityManagerTest.cpp
        ipf/OptionChainsTest.cpp
        ipf/OptionSeriesTest.cpp
        model/IndexedTxModelTest.cpp
        model/TimeSeriesTxModelTest.cpp
        model/TxModelTapeTest.cpp
        model/MarketDepthModelTest.cpp
        promise/PromisesTest.cpp
        schedule/ScheduleTest.cpp
        symbols/SymbolWrapperTest.cpp
        system/SystemTest.cpp)
#add_definitions(-DDXFCPP_DEBUG -DDXFCPP_DEBUG_ISOLATES)

if (DXFCXX_BUILD_BENCHMARKS)
    list(APPEND DXFC_TEST_SOURCES bench/StringLikeBench.cpp)
endif ()

# Tests that need external network access (demo.dxfeed.com). They are labeled "network": `ctest -L network`.
if (DXFCXX_ENABLE_NETWORK_TESTS)
    list(APPEND DXFC_TEST_SOURCES network/NetworkTest.cpp)
endif ()

set(DXFC_TEST_SUPPORT_SOURCES support/TestSupport.cpp support/EventFactory.cpp support/TapeSupport.cpp)

# Temporary files of the tests (tapes written by the tests).
set(DXFC_TEST_TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${DXFC_TEST_TEMP_DIR}")

foreach (DXFC_TEST_SOURCE ${DXFC_TEST_SOURCES})
    string(REPLACE ".cpp" "" DXFC_TEST_BASENAME "${DXFC_TEST_SOURCE}")
    string(REPLACE "/" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    string(REPLACE "\\" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    set(DXFC_TEST_BASENAME dxFeedGraalCxxApi_${DXFC_TEST_BASENAME})
    #    message("${DXFC_TEST_BASENAME}: ${DXFC_TEST_SOURCE}")

    add_executable(${DXFC_TEST_BASENAME} ${DXFC_TEST_SOURCE} main.cpp ${DXFC_TEST_SUPPORT_SOURCES})
    target_include_directories(${DXFC_TEST_BASENAME} PRIVATE ${DXFC_TEST_INCLUDE_DIRS})
    target_compile_definitions(${DXFC_TEST_BASENAME} PRIVATE
            DXFCXX_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" DXFCXX_TEST_TEMP_DIR="${DXFC_TEST_TEMP_DIR}")
    target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE fmt::fmt-header-only)

    if (WIN32)
        target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE ws2_32)
    endif ()

    if (DXFC_TEST_BASENAME MATCHES "Bench$")
        target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE nanobench)
    endif ()

    if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS)
        target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE dxfcxx)
        target_compile_definitions(${DXFC_TEST_BASENAME} PRIVATE DXFCPP_USE_DLLS)
    else ()
        target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE dxfcxx::static)

        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            if (DXFCXX_LINK_STATIC_RUNTIME)
                set_target_properties(${DXFC_TEST_BASENAME} PROPERTIES
                        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
                )
            endif ()
        endif ()
    endif ()

    # Workaround for mingw + debug
    target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE dxfcxx_mingw_debug_link_options_interface)

    if (DXFCXX_FEATURE_STACKTRACE)
        LinkStacktrace(${DXFC_TEST_BASENAME})
        target_compile_definitions(${DXFC_TEST_BASENAME} PUBLIC DXFCXX_FEATURE_STACKTRACE)
    endif ()

    if (DXFCXX_TRACING)
        target_compile_definitions(${DXFC_TEST_BASENAME} PUBLIC DXFCPP_DEBUG=1)

        if (DXFCXX_LIST_TRACING)
            target_compile_definitions(${DXFC_TEST_BASENAME} PUBLIC DXFCPP_TRACE_LISTS=1)
        endif ()

        if (DXFCXX_ISOLATES_TRACING)
            target_compile_definitions(${DXFC_TEST_BASENAME} PUBLIC DXFCPP_TRACE_ISOLATES=1)
        endif ()
    endif ()

    set_target_properties(${DXFC_TEST_BASENAME} PROPERTIES
            CXX_STANDARD 20
            CMAKE_C_STANDARD 11
            CXX_EXTENSIONS OFF
    )

    add_test(NAME ${DXFC_TEST_BASENAME} COMMAND ${DXFC_TEST_BASENAME})

    if (DXFC_TEST_SOURCE MATCHES "^network/")
        set_tests_properties(${DXFC_TEST_BASENAME} PROPERTIES LABELS network)
    endif ()

    if (DXFCXX_SANITIZERS_ENABLED)
        set_tests_properties(${DXFC_TEST_BASENAME} PROPERTIES ENVIRONMENT
                "UBSAN_OPTIONS=print_stacktrace=1;TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/tsan.supp:second_deadlock_stack=1;LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/lsan.supp")
    endif ()

    dxfcxx_copy_sanitizer_runtime(${DXFC_TEST_BASENAME})

    add_custom_command(TARGET ${DXFC_TEST_BASENAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx::graal>
            $<TARGET_FILE_DIR:${DXFC_TEST_BASENAME}>)

    if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS)
        add_custom_command(TARGET ${DXFC_TEST_BASENAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:dxfcxx>
                $<TARGET_FILE_DIR:${DXFC_TEST_BASENAME}>)
    endif ()
endforeach ()

# Sanitizer canaries: a deliberately defective LIBRARY function must produce the report of each active sanitizer.
# They fail when a sanitizer is silently not applied, or when the library itself is not instrumented.
if (DXFCXX_SANITIZERS_ENABLED)
    set(DXFC_CANARY_TARGET dxFeedGraalCxxApi_SanitizerCanary)
    add_executable(${DXFC_CANARY_TARGET} sanitizers/SanitizerCanaryTest.cpp)

    if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS)
        target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx)
        target_compile_definitions(${DXFC_CANARY_TARGET} PRIVATE DXFCPP_USE_DLLS)
        add_custom_command(TARGET ${DXFC_CANARY_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:dxfcxx> $<TARGET_FILE_DIR:${DXFC_CANARY_TARGET}>)
    else ()
        target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx::static)

        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND DXFCXX_LINK_STATIC_RUNTIME)
            set_target_properties(${DXFC_CANARY_TARGET} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif ()
    endif ()

    target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx_mingw_debug_link_options_interface)
    dxfcxx_copy_sanitizer_runtime(${DXFC_CANARY_TARGET})
    add_custom_command(TARGET ${DXFC_CANARY_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx::graal> $<TARGET_FILE_DIR:${DXFC_CANARY_TARGET}>)

    # abort_on_error=0: exit with a code instead of abort(). The default is 1 on macOS, and CTest reports a test killed by
    # a signal as failed even when its output matches PASS_REGULAR_EXPRESSION.
    set(DXFC_CANARY_ENVIRONMENT
            "ASAN_OPTIONS=abort_on_error=0"
            "UBSAN_OPTIONS=print_stacktrace=1:abort_on_error=0"
            "TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/tsan.supp:abort_on_error=0")

    if (DXFCXX_ASAN_ACTIVE)
        add_test(NAME ${DXFC_CANARY_TARGET}_asan COMMAND ${DXFC_CANARY_TARGET} asan)
        set_tests_properties(${DXFC_CANARY_TARGET}_asan PROPERTIES ENVIRONMENT "${DXFC_CANARY_ENVIRONMENT}"
                PASS_REGULAR_EXPRESSION "AddressSanitizer: heap-buffer-overflow")
    endif ()

    if (DXFCXX_UBSAN_ACTIVE)
        add_test(NAME ${DXFC_CANARY_TARGET}_ubsan COMMAND ${DXFC_CANARY_TARGET} ubsan)
        set_tests_properties(${DXFC_CANARY_TARGET}_ubsan PROPERTIES ENVIRONMENT "${DXFC_CANARY_ENVIRONMENT}"
                PASS_REGULAR_EXPRESSION "runtime error: signed integer overflow")
    endif ()

    if (DXFCXX_TSAN_ACTIVE)
        add_test(NAME ${DXFC_CANARY_TARGET}_tsan COMMAND ${DXFC_CANARY_TARGET} tsan)
        set_tests_properties(${DXFC_CANARY_TARGET}_tsan PROPERTIES ENVIRONMENT "${DXFC_CANARY_ENVIRONMENT}"
                PASS_REGULAR_EXPRESSION "ThreadSanitizer: data race")
    endif ()
endif ()
