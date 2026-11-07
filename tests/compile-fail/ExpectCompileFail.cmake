# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Builds one target of the compile-fail directory and checks the outcome.
#   -DBUILD_DIR=<build tree> -DTARGET=<target> [-DCONFIG=<config>] [-DEXPECT=fail|pass] [-DMARKER=<text>]
# EXPECT=fail (default): the build must fail, and some diagnostic line (an "error" or its "note", as Clang explains the
#   rejected conversion in a note; MSBuild prints a note of clang-cl as "message") must mention MARKER (default
#   StringParam). A missing include file mentions StringParam.hpp too, so that name is not counted: a broken harness
#   cannot pass vacuously.
# EXPECT=pass: the build must succeed (a positive control of the same harness).
if (NOT DEFINED EXPECT)
    set(EXPECT fail)
endif ()

if (NOT DEFINED MARKER)
    set(MARKER StringParam)
endif ()

set(build_args --build "${BUILD_DIR}" --target "${TARGET}")

if (CONFIG)
    list(APPEND build_args --config "${CONFIG}")
endif ()

execute_process(COMMAND "${CMAKE_COMMAND}" ${build_args}
        RESULT_VARIABLE exit_code OUTPUT_VARIABLE output ERROR_VARIABLE output)

if (EXPECT STREQUAL "pass")
    if (NOT exit_code EQUAL 0)
        message(FATAL_ERROR "${TARGET}: expected to compile, the build failed (${exit_code}):\n${output}")
    endif ()

    message(STATUS "${TARGET}: compiled as expected")
    return()
endif ()

if (exit_code EQUAL 0)
    message(FATAL_ERROR "${TARGET}: expected a compile error, but it compiled:\n${output}")
endif ()

string(REPLACE "${MARKER}.hpp" "" cleaned "${output}")
string(REGEX MATCH "[^\n]*([eE][rR][rR][oO][rR]|[nN][oO][tT][eE]|[mM][eE][sS][sS][aA][gG][eE])[^\n]*${MARKER}[^\n]*"
        diagnostic "${cleaned}")

if (NOT diagnostic)
    message(FATAL_ERROR
            "${TARGET}: the build failed (${exit_code}), but no error, note or message line mentions ${MARKER}:\n${output}")
endif ()

message(STATUS "${TARGET}: compile error as expected: ${diagnostic}")
