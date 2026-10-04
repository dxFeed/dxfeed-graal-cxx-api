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
    # GCC reports -Wmaybe-uninitialized in third-party code inlined into ours (range-v3 with ASan in Release), even from
    # system headers, and documents its false positives with sanitizers: it stays a warning.
    target_compile_options(dxfcxx_warnings INTERFACE
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Werror>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-error=maybe-uninitialized>"
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX>")
endif ()

# Clang 22+ reports __COUNTER__ with -pedantic as a C2y extension at the macro use site, even when the macro comes from a
# system header: every TEST_CASE of doctest. The targets that use doctest add DXFCXX_DOCTEST_COMPILE_OPTIONS. Older
# Clang versions do not know the flag and report it as unknown, hence the check.
set(DXFCXX_DOCTEST_COMPILE_OPTIONS "")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-Wc2y-extensions DXFCXX_HAS_WC2Y_EXTENSIONS)

    if (DXFCXX_HAS_WC2Y_EXTENSIONS)
        set(DXFCXX_DOCTEST_COMPILE_OPTIONS -Wno-c2y-extensions)
    endif ()
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

# The same for all targets of a dependency added as a directory tree (for example, Boost with its libraries).
function(dxfcxx_system_includes_in_directory directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    dxfcxx_system_includes(${targets})
    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)

    foreach (subdirectory IN LISTS subdirectories)
        dxfcxx_system_includes_in_directory("${subdirectory}")
    endforeach ()
endfunction()
