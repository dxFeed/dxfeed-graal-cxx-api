# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks that the build options of the library do not reach the project that uses it: the compile commands of the
# project's own sources (compile_commands.json of DXFCXX_CONSUMER_BINARY_DIR) have none of the options and definitions
# the library builds itself with.
# Usage: cmake -DDXFCXX_CONSUMER_BINARY_DIR=<dir> -P check-options.cmake

set(commandsFile "${DXFCXX_CONSUMER_BINARY_DIR}/compile_commands.json")

if (NOT EXISTS "${commandsFile}")
    message(FATAL_ERROR "No ${commandsFile}")
endif ()

set(libraryOptions
        /bigobj /utf-8 /W4 /w44365 /sdl /guard:cf
        -Wall -Wextra -pedantic -Wshadow -Wsign-conversion -fPIC noexecstack
        _FORTIFY_SOURCE -fstack-protector -fstack-clash-protection -fcf-protection
        DXFCXX_VERSION LIBDXFCPP_EXPORTS FMT_HEADER_ONLY DXFCXX_EXPORT_INTERNALS)

file(READ "${commandsFile}" commands)
string(JSON count LENGTH "${commands}")
math(EXPR last "${count} - 1")
set(checked 0)
set(found "")

foreach (i RANGE ${last})
    string(JSON file GET "${commands}" ${i} file)

    # The project's own sources only: in the subdirectory modes the library's sources are there too.
    if (NOT file MATCHES "(package-consumer-check/types|static-mt-check/main)\\.cpp$")
        continue()
    endif ()

    string(JSON command ERROR_VARIABLE noCommand GET "${commands}" ${i} command)

    if (noCommand)
        string(JSON command GET "${commands}" ${i} arguments)
    endif ()

    math(EXPR checked "${checked} + 1")

    foreach (option IN LISTS libraryOptions)
        string(FIND "${command}" "${option}" position)

        if (NOT position EQUAL -1)
            list(APPEND found "${file}: ${option}")
        endif ()
    endforeach ()
endforeach ()

if (checked EQUAL 0)
    message(FATAL_ERROR "No compile commands of the project's own sources in ${commandsFile}")
endif ()

if (found)
    list(JOIN found "\n  " foundList)
    message(FATAL_ERROR "Options of the library in the project's compile commands:\n  ${foundList}")
endif ()

message(STATUS "${checked} compile commands without the options of the library")
