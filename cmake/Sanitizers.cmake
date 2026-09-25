# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Sanitizer support.
#
# `dxfcxx_sanitizers` is an INTERFACE target that carries the sanitizer compile and link options. It is linked PUBLIC to
# both library targets, so the library code itself is instrumented and every consumer (tests, samples, tools) is
# instrumented too and links the sanitizer runtime.
#
# Options (declared in the root CMakeLists.txt):
#   DXFCXX_ENABLE_ASAN  - AddressSanitizer (+ LeakSanitizer where available). GCC, Clang, AppleClang, MSVC, MinGW Clang.
#   DXFCXX_ENABLE_UBSAN - UndefinedBehaviorSanitizer; can be combined with ASan. Not supported by MSVC.
#   DXFCXX_ENABLE_TSAN  - ThreadSanitizer; cannot be combined with ASan. GCC, Clang, AppleClang.
# MinGW GCC has no sanitizer runtime (libasan/libubsan), so the options are ignored there with a warning; use the
# MSYS2 clang64 toolchain for sanitized Windows builds with MinGW.
#
# Sanitized builds define DXFCXX_SANITIZERS_ENABLED=1.

add_library(dxfcxx_sanitizers INTERFACE)

if (DXFCXX_ENABLE_TSAN AND DXFCXX_ENABLE_ASAN)
    message(FATAL_ERROR "DXFCXX_ENABLE_TSAN cannot be combined with DXFCXX_ENABLE_ASAN")
endif ()

set(DXFCXX_SANITIZERS_ENABLED OFF)

if (MSVC)
    if (DXFCXX_ENABLE_UBSAN)
        message(WARNING "DXFCXX_ENABLE_UBSAN is ignored: MSVC does not support UndefinedBehaviorSanitizer")
    endif ()

    if (DXFCXX_ENABLE_TSAN)
        message(WARNING "DXFCXX_ENABLE_TSAN is ignored: MSVC does not support ThreadSanitizer")
    endif ()

    if (DXFCXX_ENABLE_ASAN)
        set(DXFCXX_SANITIZERS_ENABLED ON)

        # /Z7 + /DEBUG: debug information for readable ASan reports in every configuration (warning C5072 otherwise).
        # /Z7 keeps it in the object files, so parallel compiler processes do not share a PDB (C1041 with /Zi);
        # the linker still writes the PDB of each executable and DLL.
        target_compile_options(dxfcxx_sanitizers INTERFACE /fsanitize=address /Z7)
        # ASan is incompatible with incremental linking and with the /RTC run-time checks of the Debug configuration.
        target_link_options(dxfcxx_sanitizers INTERFACE /INCREMENTAL:NO /DEBUG)
        # Third-party static libraries (Process, Console, ...) are not instrumented. Disable all STL ASan annotations,
        # otherwise the linker reports `mismatch detected for 'annotate_<container>'` (newer STLs annotate
        # std::optional too) and std::vector/std::string that cross the boundary give false container-overflows.
        target_compile_definitions(dxfcxx_sanitizers INTERFACE _DISABLE_STL_ANNOTATION)

        foreach (flagVar CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "/RTC[1csu]*" "" ${flagVar} "${${flagVar}}")
            string(REPLACE "/Zi" "/Z7" ${flagVar} "${${flagVar}}")
        endforeach ()
    endif ()
elseif (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (DXFCXX_ENABLE_ASAN OR DXFCXX_ENABLE_UBSAN OR DXFCXX_ENABLE_TSAN)
        message(WARNING "Sanitizers are ignored: MinGW GCC has no sanitizer runtime; use the MSYS2 clang64 toolchain")
    endif ()
else ()
    set(sanitizers "")

    if (DXFCXX_ENABLE_ASAN)
        list(APPEND sanitizers address)
    endif ()

    if (DXFCXX_ENABLE_UBSAN)
        list(APPEND sanitizers undefined)
    endif ()

    if (DXFCXX_ENABLE_TSAN)
        list(APPEND sanitizers thread)
    endif ()

    if (sanitizers)
        set(DXFCXX_SANITIZERS_ENABLED ON)
        list(JOIN sanitizers "," sanitizers)

        # -fno-sanitize-recover=all: a UBSan report terminates the test instead of being printed and ignored.
        target_compile_options(dxfcxx_sanitizers INTERFACE
                -fsanitize=${sanitizers} -fno-sanitize-recover=all -fno-omit-frame-pointer -g)
        target_link_options(dxfcxx_sanitizers INTERFACE -fsanitize=${sanitizers})
    endif ()
endif ()

if (DXFCXX_SANITIZERS_ENABLED)
    target_compile_definitions(dxfcxx_sanitizers INTERFACE DXFCXX_SANITIZERS_ENABLED=1)
endif ()

# Copies the MSVC ASan runtime DLL next to an executable, so it can be started outside a Visual Studio developer
# prompt (for example by CTest on a CI runner). Does nothing for other compilers or without ASan.
function(dxfcxx_copy_sanitizer_runtime targetName)
    if (NOT (MSVC AND DXFCXX_ENABLE_ASAN))
        return()
    endif ()

    get_filename_component(msvcBinDir "${CMAKE_CXX_COMPILER}" DIRECTORY)

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(arch x86_64)
    else ()
        set(arch i386)
    endif ()

    # Which runtime is linked (clang_rt.asan_dynamic or clang_rt.asan_dbg_dynamic for /MDd) depends on the MSVC
    # version, so all variants found next to the compiler are copied.
    file(GLOB runtimes "${msvcBinDir}/clang_rt.asan*_dynamic-${arch}.dll")

    if (NOT runtimes)
        message(WARNING "MSVC ASan runtime DLL not found in ${msvcBinDir}")
        return()
    endif ()

    add_custom_command(TARGET ${targetName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtimes} "$<TARGET_FILE_DIR:${targetName}>")
endfunction()
