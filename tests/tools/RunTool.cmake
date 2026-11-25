# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Runs a tool and checks its exit code and its output; CTest checks only one of them when a test has a regex.
# Usage: cmake -DTOOL=<path> -DARG0=<arg> [-DARG1=<arg> ... -DARG11=<arg>] -DEXPECTED_EXIT=<code>
#              -DEXPECTED_OUTPUT=<regex> -P RunTool.cmake

set(TOOL_ARGS)

foreach (INDEX RANGE 0 11)
    if (DEFINED ARG${INDEX})
        list(APPEND TOOL_ARGS "${ARG${INDEX}}")
    endif ()
endforeach ()

execute_process(COMMAND "${TOOL}" ${TOOL_ARGS} RESULT_VARIABLE EXIT_CODE OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE OUTPUT)

# The output of the tool for the FAIL_REGULAR_EXPRESSION of the test (the arguments are not printed: they may have
# secrets).
message("${OUTPUT}")

if (NOT "${EXIT_CODE}" STREQUAL "${EXPECTED_EXIT}")
    message(FATAL_ERROR "The exit code is ${EXIT_CODE}, expected ${EXPECTED_EXIT}")
endif ()

if (NOT OUTPUT MATCHES "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "The output does not match: ${EXPECTED_OUTPUT}")
endif ()
