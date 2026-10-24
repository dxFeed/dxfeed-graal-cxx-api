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

set(DXFC_TEST_INCLUDE_DIRS ../include)
set(DXFC_TEST_SYSTEM_INCLUDE_DIRS ../third_party/doctest-${DOCTEST_VERSION} ../third_party/range-v3-${RANGE_VERSION}/include)

if (DXFCXX_BUILD_BENCHMARKS)
    FetchContent_Declare(
            nanobench
            GIT_REPOSITORY https://github.com/martinus/nanobench.git
            GIT_TAG a5a50c2b33eea2ff1fcb355cacdface43eb42b25 # v4.3.11
    )
    FetchContent_MakeAvailable(nanobench)
    dxfcxx_system_includes(nanobench)
endif ()

set(DXFC_TEST_SOURCES
        api/CandleAttributesTest.cpp
        api/CandleDataTypeTest.cpp
        api/CandleSymbolTest.cpp
        api/DataIntegrityTest.cpp
        api/DXFeedTest.cpp
        api/DXEndpointTest.cpp
        api/DXPublisherTest.cpp
        api/EqualityTest.cpp
        api/EventsTest.cpp
        api/ExportsTest.cpp
        api/MarketEventSymbolsTest.cpp
        api/MarketMakerTest.cpp
        api/NuamEventsTest.cpp
        api/ObservableSubscriptionLifecycleTest.cpp
        api/OrderImbalanceTest.cpp
        api/OrderSourceTest.cpp
        api/TextConfigurationTest.cpp
        event/ConstantsTest.cpp
        event/EdgeValuesTapeTest.cpp
        event/EventsTest.cpp
        event/EventTypeTest.cpp
        event/TapeRoundTripTest.cpp
        exceptions/ExceptionsTest.cpp
        glossary/AdditionalUnderlyingsTest.cpp
        glossary/PriceIncrementsTest.cpp
        internal/CommonTest.cpp
        internal/ConstStringTest.cpp
        internal/EntityManagerTest.cpp
        internal/HandlerTest.cpp
        ipf/InstrumentProfileCollectorTest.cpp
        ipf/InstrumentProfileTest.cpp
        ipf/OptionChainsTest.cpp
        ipf/OptionSeriesTest.cpp
        model/IndexedEventTxModelTest.cpp
        model/IndexedTxModelTest.cpp
        model/TimeSeriesTxModelTest.cpp
        model/TxModelTapeTest.cpp
        model/MarketDepthModelTest.cpp
        orcs/PriceLevelServiceTest.cpp
        promise/PromisesTest.cpp
        schedule/ScheduleTest.cpp
        symbols/SymbolWrapperTest.cpp
        system/SystemTest.cpp
        tools/ToolsArgsTest.cpp)
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

    # The argument parsing of the tools is compiled into their tests: the tools are an executable.
    if (DXFC_TEST_SOURCE MATCHES "^tools/")
        target_sources(${DXFC_TEST_BASENAME} PRIVATE ../tools/Tools/src/Args/Args.cpp)
    endif ()
    list(APPEND DXFC_TEST_TARGETS ${DXFC_TEST_BASENAME})
    target_include_directories(${DXFC_TEST_BASENAME} PRIVATE ${DXFC_TEST_INCLUDE_DIRS})
    target_include_directories(${DXFC_TEST_BASENAME} SYSTEM PRIVATE ${DXFC_TEST_SYSTEM_INCLUDE_DIRS})
    target_compile_options(${DXFC_TEST_BASENAME} PRIVATE ${DXFCXX_DOCTEST_COMPILE_OPTIONS})
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

        if (MSVC)
            if (DXFCXX_LINK_STATIC_RUNTIME)
                set_target_properties(${DXFC_TEST_BASENAME} PROPERTIES
                        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
                )
            endif ()
        endif ()
    endif ()

    # Workaround for mingw + debug
    target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE dxfcxx_mingw_debug_link_options_interface dxfcxx_warnings)

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
endforeach ()

# Sanitizer canaries: a deliberately defective LIBRARY function must produce the report of each active sanitizer.
# They fail when a sanitizer is silently not applied, or when the library itself is not instrumented.
if (DXFCXX_SANITIZERS_ENABLED)
    set(DXFC_CANARY_TARGET dxFeedGraalCxxApi_SanitizerCanary)
    add_executable(${DXFC_CANARY_TARGET} sanitizers/SanitizerCanaryTest.cpp)
    list(APPEND DXFC_TEST_TARGETS ${DXFC_CANARY_TARGET})

    if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS)
        target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx)
        target_compile_definitions(${DXFC_CANARY_TARGET} PRIVATE DXFCPP_USE_DLLS)
    else ()
        target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx::static)

        if (MSVC AND DXFCXX_LINK_STATIC_RUNTIME)
            set_target_properties(${DXFC_CANARY_TARGET} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif ()
    endif ()

    target_link_libraries(${DXFC_CANARY_TARGET} PRIVATE dxfcxx_mingw_debug_link_options_interface)

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

# The library constants read from the global initializers of a user translation unit, with each kind of linking.
# The shared variant is not built with the static C runtime: the library and the test would have separate heaps. The
# static_nograal variant reads only the constants that do not need Java and checks that the Graal isolate is not created.
set(DXFC_STATIC_INIT_VARIANTS static static_nograal)

if (NOT (MSVC AND DXFCXX_LINK_STATIC_RUNTIME))
    list(APPEND DXFC_STATIC_INIT_VARIANTS shared)
endif ()

foreach (DXFC_STATIC_INIT_VARIANT IN LISTS DXFC_STATIC_INIT_VARIANTS)
    set(DXFC_STATIC_INIT_TARGET dxFeedGraalCxxApi_StaticInit_${DXFC_STATIC_INIT_VARIANT})
    add_executable(${DXFC_STATIC_INIT_TARGET} static-init/StaticInitTest.cpp)
    list(APPEND DXFC_TEST_TARGETS ${DXFC_STATIC_INIT_TARGET})
    target_include_directories(${DXFC_STATIC_INIT_TARGET} PRIVATE ${DXFC_TEST_INCLUDE_DIRS})

    if (DXFC_STATIC_INIT_VARIANT STREQUAL "shared")
        target_link_libraries(${DXFC_STATIC_INIT_TARGET} PRIVATE dxfcxx)
        target_compile_definitions(${DXFC_STATIC_INIT_TARGET} PRIVATE DXFCPP_USE_DLLS)
    else ()
        target_link_libraries(${DXFC_STATIC_INIT_TARGET} PRIVATE dxfcxx::static)

        if (DXFC_STATIC_INIT_VARIANT STREQUAL "static_nograal")
            target_compile_definitions(${DXFC_STATIC_INIT_TARGET} PRIVATE DXFC_STATIC_INIT_NO_GRAAL)
        endif ()

        if (MSVC AND DXFCXX_LINK_STATIC_RUNTIME)
            set_target_properties(${DXFC_STATIC_INIT_TARGET} PROPERTIES
                    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif ()
    endif ()

    target_link_libraries(${DXFC_STATIC_INIT_TARGET} PRIVATE dxfcxx_mingw_debug_link_options_interface dxfcxx_warnings)
    set_target_properties(${DXFC_STATIC_INIT_TARGET} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)
    add_test(NAME ${DXFC_STATIC_INIT_TARGET} COMMAND ${DXFC_STATIC_INIT_TARGET})

    if (DXFCXX_SANITIZERS_ENABLED)
        set_tests_properties(${DXFC_STATIC_INIT_TARGET} PROPERTIES ENVIRONMENT
                "ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1;UBSAN_OPTIONS=print_stacktrace=1;TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/tsan.supp;LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/lsan.supp")
    endif ()
endforeach ()

# The process exits while the Graal threads are running, with each kind of linking; a hang at exit fails the test by its
# timeout. The shared variant is not built with the static C runtime, as above.
set(DXFC_EXIT_VARIANTS static)

if (NOT (MSVC AND DXFCXX_LINK_STATIC_RUNTIME))
    list(APPEND DXFC_EXIT_VARIANTS shared)
endif ()

foreach (DXFC_EXIT_VARIANT IN LISTS DXFC_EXIT_VARIANTS)
    set(DXFC_EXIT_TARGET dxFeedGraalCxxApi_Exit_${DXFC_EXIT_VARIANT})
    set(DXFC_EXIT_SCENARIOS endpoint ipf-live ipf-live-close shutdown)

    # The exit handler of a MinGW DLL runs after the statics of the executable are destroyed, so the checks of the
    # callbacks from such a static do not apply to it.
    if (DXFC_EXIT_VARIANT STREQUAL "static" OR NOT WIN32 OR MSVC)
        list(APPEND DXFC_EXIT_SCENARIOS late-callback callback-in-progress)
    endif ()
    add_executable(${DXFC_EXIT_TARGET} exit/ExitTest.cpp)
    list(APPEND DXFC_TEST_TARGETS ${DXFC_EXIT_TARGET})
    target_include_directories(${DXFC_EXIT_TARGET} PRIVATE ${DXFC_TEST_INCLUDE_DIRS})

    if (DXFC_EXIT_VARIANT STREQUAL "shared")
        target_link_libraries(${DXFC_EXIT_TARGET} PRIVATE dxfcxx)
        target_compile_definitions(${DXFC_EXIT_TARGET} PRIVATE DXFCPP_USE_DLLS)
    else ()
        target_link_libraries(${DXFC_EXIT_TARGET} PRIVATE dxfcxx::static)

        if (MSVC AND DXFCXX_LINK_STATIC_RUNTIME)
            set_target_properties(${DXFC_EXIT_TARGET} PROPERTIES
                    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif ()
    endif ()

    if (WIN32)
        target_link_libraries(${DXFC_EXIT_TARGET} PRIVATE ws2_32)
    endif ()

    target_link_libraries(${DXFC_EXIT_TARGET} PRIVATE dxfcxx_mingw_debug_link_options_interface dxfcxx_warnings)
    set_target_properties(${DXFC_EXIT_TARGET} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

    foreach (DXFC_EXIT_SCENARIO IN LISTS DXFC_EXIT_SCENARIOS)
        set(DXFC_EXIT_TEST ${DXFC_EXIT_TARGET}_${DXFC_EXIT_SCENARIO})
        add_test(NAME ${DXFC_EXIT_TEST} COMMAND ${DXFC_EXIT_TARGET} ${DXFC_EXIT_SCENARIO})
        set_tests_properties(${DXFC_EXIT_TEST} PROPERTIES TIMEOUT 60)

        if (DXFCXX_SANITIZERS_ENABLED)
            set_tests_properties(${DXFC_EXIT_TEST} PROPERTIES ENVIRONMENT
                    "UBSAN_OPTIONS=print_stacktrace=1;TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/tsan.supp;LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizers/lsan.supp")
        endif ()
    endforeach ()
endforeach ()

# The libraries that the tests and the canary load, copied next to them once (see dxfcxx_copy_runtime).
set(DXFC_TEST_RUNTIME_FILES $<TARGET_FILE:dxfcxx::graal>)
set(DXFC_TEST_RUNTIME_DEPENDS "")

if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS OR "shared" IN_LIST DXFC_STATIC_INIT_VARIANTS)
    list(APPEND DXFC_TEST_RUNTIME_FILES $<TARGET_FILE:dxfcxx>)
    list(APPEND DXFC_TEST_RUNTIME_DEPENDS dxfcxx)
endif ()

dxfcxx_sanitizer_runtime_files(DXFC_SANITIZER_RUNTIME_FILES)
list(APPEND DXFC_TEST_RUNTIME_FILES ${DXFC_SANITIZER_RUNTIME_FILES})
dxfcxx_copy_runtime(dxfcxx_test_runtime FILES ${DXFC_TEST_RUNTIME_FILES} TARGETS ${DXFC_TEST_TARGETS}
        DEPENDS ${DXFC_TEST_RUNTIME_DEPENDS})
