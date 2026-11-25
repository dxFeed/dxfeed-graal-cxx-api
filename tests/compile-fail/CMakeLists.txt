# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Compile-fail tests: a call that must not compile. Each case is an object target excluded from the normal build; its
# test builds it and expects a compile error that names dxfcpp::StringParam (see ExpectCompileFail.cmake). The control
# case must compile, so a broken harness (wrong include directories, flags) cannot make the negative tests pass.
# Tests of one build tree run one at a time: they run the same build.
set(DXFC_COMPILE_FAIL_CASES Int OptionalString WString)

function(dxfcxx_add_compile_case name expect)
    set(target dxFeedGraalCxxApi_CompileFail_${name})
    add_library(${target} OBJECT EXCLUDE_FROM_ALL ${name}.cpp)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(${target} PRIVATE dxfcxx_warnings)
    set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    add_test(NAME ${target}
            COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DTARGET=${target} -DCONFIG=$<CONFIG>
            -DEXPECT=${expect} -P ${CMAKE_CURRENT_SOURCE_DIR}/ExpectCompileFail.cmake)
    set_tests_properties(${target} PROPERTIES RESOURCE_LOCK dxfcxx_compile_fail LABELS compile-fail TIMEOUT 300)
endfunction()

foreach (DXFC_COMPILE_FAIL_CASE IN LISTS DXFC_COMPILE_FAIL_CASES)
    dxfcxx_add_compile_case(${DXFC_COMPILE_FAIL_CASE} fail)
endforeach ()

dxfcxx_add_compile_case(Control pass)
