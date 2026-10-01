# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# The compiler warnings of this project's own code (the libraries, tests, samples, tools, fuzzers) on top of the
# warnings of the libraries (-Wall -Wextra -pedantic, /W4). They come from the dxfcxx_warnings interface target, which
# is linked PRIVATE, so they do not reach the projects that use the library. MSVC: C4365 is the signed/unsigned
# conversion warning (-Wsign-conversion); the shadowing warnings C4456-C4459 are in /W4 already.
option(DXFCXX_WARNINGS_AS_ERRORS
        "Treat the compiler warnings of this project's own code as errors (not of the dependencies)" OFF)

add_library(dxfcxx_warnings INTERFACE)
target_compile_options(dxfcxx_warnings INTERFACE
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wshadow;-Wsign-conversion;-Wfloat-equal;-Wextra-semi>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/w44365>")

if (DXFCXX_WARNINGS_AS_ERRORS)
    target_compile_options(dxfcxx_warnings INTERFACE
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Werror>"
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX>")
endif ()

# Makes the include directories of the given dependency targets system ones, so that the warnings in their headers are
# not reported in this project's code. Missing targets are skipped; an ALIAS is resolved to its target.
function(dxfcxx_system_includes)
    foreach (target IN LISTS ARGN)
        if (NOT TARGET ${target})
            continue()
        endif ()

        get_target_property(aliased ${target} ALIASED_TARGET)

        if (aliased)
            set(target ${aliased})
        endif ()

        get_target_property(directories ${target} INTERFACE_INCLUDE_DIRECTORIES)

        if (directories)
            set_property(TARGET ${target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${directories})
        endif ()
    endforeach ()
endfunction()
