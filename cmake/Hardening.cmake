# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# The hardening options of the compiler and the linker for the library targets (PRIVATE, so they do not reach the
# projects that use the library):
#   GCC, Clang (Linux, macOS): -D_FORTIFY_SOURCE=3 (glibc before 2.34 takes it as 2), -fstack-protector-strong,
#     -fstack-clash-protection (Linux), -fcf-protection=full (x86_64); the shared library is linked with
#     -z relro -z now (Linux).
#   MSVC: /guard:cf (Control Flow Guard), /sdl, /CETCOMPAT (x64).
# They are used in the optimized configurations only (Release, RelWithDebInfo, MinSizeRel; not in Debug or a build
# without a build type: _FORTIFY_SOURCE needs optimization) and not in sanitized builds (the
# sanitizers check the same and more, and _FORTIFY_SOURCE hides the calls from ASan). MinGW builds are not hardened.
#
# Uses DXFCXX_SANITIZERS_ENABLED (cmake/Sanitizers.cmake).
#
# dxfcxx_harden(<target>...)

option(DXFCXX_HARDENING
        "Build the libraries with the hardening options of the compiler and the linker (not in Debug or sanitized builds)"
        ON)

set(DXFCXX_HARDENING_COMPILE_OPTIONS "")
set(DXFCXX_HARDENING_LINK_OPTIONS "")

if (DXFCXX_HARDENING AND NOT DXFCXX_SANITIZERS_ENABLED)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND DXFCXX_HARDENING_COMPILE_OPTIONS /guard:cf /sdl)
        list(APPEND DXFCXX_HARDENING_LINK_OPTIONS /guard:cf)

        if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
            list(APPEND DXFCXX_HARDENING_LINK_OPTIONS /CETCOMPAT)
        endif ()
    elseif (NOT WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        include(CheckCXXCompilerFlag)

        list(APPEND DXFCXX_HARDENING_COMPILE_OPTIONS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -fstack-protector-strong)

        if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            check_cxx_compiler_flag(-fstack-clash-protection DXFCXX_HAS_STACK_CLASH_PROTECTION)

            if (DXFCXX_HAS_STACK_CLASH_PROTECTION)
                list(APPEND DXFCXX_HARDENING_COMPILE_OPTIONS -fstack-clash-protection)
            endif ()

            list(APPEND DXFCXX_HARDENING_LINK_OPTIONS -Wl,-z,relro,-z,now)
        endif ()

        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
            check_cxx_compiler_flag(-fcf-protection=full DXFCXX_HAS_CF_PROTECTION)

            if (DXFCXX_HAS_CF_PROTECTION)
                list(APPEND DXFCXX_HARDENING_COMPILE_OPTIONS -fcf-protection=full)
            endif ()
        endif ()
    endif ()
endif ()

function(dxfcxx_harden)
    # The optimized configurations only: a build without a build type is not optimized either.
    set(optimized "$<CONFIG:Release,RelWithDebInfo,MinSizeRel>")

    foreach (target IN LISTS ARGN)
        target_compile_options(${target} PRIVATE
                "$<$<AND:$<COMPILE_LANGUAGE:CXX>,${optimized}>:${DXFCXX_HARDENING_COMPILE_OPTIONS}>")

        get_target_property(type ${target} TYPE)

        if (type STREQUAL "SHARED_LIBRARY")
            target_link_options(${target} PRIVATE "$<${optimized}:${DXFCXX_HARDENING_LINK_OPTIONS}>")
        endif ()
    endforeach ()
endfunction()
