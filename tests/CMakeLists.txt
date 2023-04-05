# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

project(dxFeedGraalCXXAPITests LANGUAGES CXX)

set(DXFC_TEST_INCLUDE_DIRS ../include ../third_party/doctest-2.4.11)

set(DXFC_TEST_SOURCES system/SystemTest.cpp api/DXEndpointTest.cpp)
#add_definitions(-DDXFCPP_DEBUG -DDXFCPP_DEBUG_ISOLATES)

foreach (DXFC_TEST_SOURCE ${DXFC_TEST_SOURCES})
    string(REPLACE ".cpp" "" DXFC_TEST_BASENAME "${DXFC_TEST_SOURCE}")
    string(REPLACE "/" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    string(REPLACE "\\" "_" DXFC_TEST_BASENAME "${DXFC_TEST_BASENAME}")
    set(DXFC_TEST_BASENAME dxFeedGraalCxxApi_${DXFC_TEST_BASENAME})
    message("${DXFC_TEST_BASENAME}: ${DXFC_TEST_SOURCE}")

    add_executable(${DXFC_TEST_BASENAME} ${DXFC_TEST_SOURCE})
    target_include_directories(${DXFC_TEST_BASENAME} PUBLIC ${DXFC_TEST_INCLUDE_DIRS})
    target_link_libraries(${DXFC_TEST_BASENAME} PUBLIC DxFeedGraalNativeApi dxFeedGraalCxxApi)
    set_property(TARGET ${DXFC_TEST_BASENAME} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${DXFC_TEST_BASENAME} PROPERTY CXX_EXTENSIONS OFF)
    add_test(NAME ${DXFC_TEST_BASENAME} COMMAND ${DXFC_TEST_BASENAME})

    add_custom_command(TARGET ${DXFC_TEST_BASENAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:DxFeedGraalNativeApi> $<TARGET_FILE_DIR:${DXFC_TEST_BASENAME}>)
endforeach ()
