# Copyright (c) 2025 Devexperts LLC.
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
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

set(DXFC_TEST_INCLUDE_DIRS ../include ../third_party/doctest-${DOCTEST_VERSION} ../third_party/range-v3-${RANGE_VERSION}/include)

if (DXFCXX_BUILD_BENCHMARKS)
    FetchContent_Declare(
            nanobench
            GIT_REPOSITORY https://github.com/martinus/nanobench.git
            GIT_TAG v4.3.11
            GIT_SHALLOW TRUE)
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
        api/OrderSourceTest.cpp
        event/EventsTest.cpp
        exceptions/ExceptionsTest.cpp
        glossary/AdditionalUnderlyingsTest.cpp
        glossary/PriceIncrementsTest.cpp
        model/IndexedTxModelTest.cpp
        model/TimeSeriesTxModelTest.cpp
        model/MarketDepthModelTest.cpp
        promise/PromisesTest.cpp
        schedule/ScheduleTest.cpp
        symbols/SymbolWrapperTest.cpp
        system/SystemTest.cpp)
#add_definitions(-DDXFCPP_DEBUG -DDXFCPP_DEBUG_ISOLATES)

if (DXFCXX_BUILD_BENCHMARKS)
    list(APPEND DXFC_TEST_SOURCES bench/StringLikeBench.cpp)
endif ()

foreach (DXFC_TEST_SOURCE ${DXFC_TEST_SOURCES})
    string(REPLACE ".cpp" "" DXFC_TEST_BASENAME "${DXFC_TEST_SOURCE}")
    string(REPLACE "/" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    string(REPLACE "\\" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    set(DXFC_TEST_BASENAME dxFeedGraalCxxApi_${DXFC_TEST_BASENAME})
    #    message("${DXFC_TEST_BASENAME}: ${DXFC_TEST_SOURCE}")

    add_executable(${DXFC_TEST_BASENAME} ${DXFC_TEST_SOURCE} main.cpp)
    target_include_directories(${DXFC_TEST_BASENAME} PRIVATE ${DXFC_TEST_INCLUDE_DIRS})
    target_link_libraries(${DXFC_TEST_BASENAME} PRIVATE fmt::fmt-header-only)

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

    if (DXFCXX_FEATURE_STACKTRACE)
        LinkStacktrace(${DXFC_TEST_BASENAME})
        target_compile_definitions(${DXFC_TEST_BASENAME} PUBLIC DXFCXX_FEATURE_STACKTRACE)
    endif ()

    if (DXFCXX_ENABLE_ASAN)
        LinkAsan(${DXFC_TEST_BASENAME})
    elseif (DXFCXX_ENABLE_UBSAN)
        LinkUbsan(${DXFC_TEST_BASENAME})
    endif ()

    set_target_properties(${DXFC_TEST_BASENAME} PROPERTIES
            CXX_STANDARD 20
            CMAKE_C_STANDARD 11
            CXX_EXTENSIONS OFF
    )

    add_test(NAME ${DXFC_TEST_BASENAME} COMMAND ${DXFC_TEST_BASENAME})

    add_custom_command(TARGET ${DXFC_TEST_BASENAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx::graal>
            $<TARGET_FILE_DIR:${DXFC_TEST_BASENAME}>)

    if (DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS)
        add_custom_command(TARGET ${DXFC_TEST_BASENAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:dxfcxx>
                $<TARGET_FILE_DIR:${DXFC_TEST_BASENAME}>)
    endif ()
endforeach ()
