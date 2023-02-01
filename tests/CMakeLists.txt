# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

project(dxFeedNativeAPITests LANGUAGES CXX)

set(DXFC_INCLUDE_DIRS ../include)

set(DXFC_TEST_SOURCES dxFeedNativeSystemTest.cpp)

string(REPLACE ".cpp" "" DXFC_TEST_BASENAMES "${DXFC_TEST_SOURCES}")
set(DXFC_TEST_TARGETS ${DXFC_TEST_BASENAMES})

foreach (dxfc_test_target ${DXFC_TEST_TARGETS})
    add_executable(${dxfc_test_target} ${dxfc_test_target}.cpp)
    target_include_directories(${dxfc_test_target} PUBLIC ${DXFC_INCLUDE_DIRS})
    target_link_libraries(${dxfc_test_target} PUBLIC DxFeedGraalNativeApi Catch2::Catch2WithMain dxFeedNativeAPI dxFeedNativeCppAPI)
    set_property(TARGET ${dxfc_test_target} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${dxfc_test_target} PROPERTY CXX_EXTENSIONS OFF)
    add_test(NAME ${dxfc_test_target} COMMAND ${dxfc_test_target})

    add_custom_command(TARGET ${dxfc_test_target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:DxFeedGraalNativeApi> $<TARGET_FILE_DIR:${dxfc_test_target}>)
endforeach ()
