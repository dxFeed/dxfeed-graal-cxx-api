# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# libFuzzer targets for the pure C++ parts of the library (no Graal isolate calls on the fuzzed paths).
# Built with -DDXFCXX_BUILD_FUZZERS=ON (Clang only), together with -DDXFCXX_ENABLE_ASAN=ON -DDXFCXX_ENABLE_UBSAN=ON so
# that memory errors and undefined behavior are found; the library is instrumented for coverage by the root
# CMakeLists.txt. See README.md for running the targets.

if (NOT DXFCXX_ASAN_ACTIVE OR NOT DXFCXX_UBSAN_ACTIVE)
    message(WARNING "DXFCXX_BUILD_FUZZERS: enable DXFCXX_ENABLE_ASAN and DXFCXX_ENABLE_UBSAN, otherwise the fuzzers "
            "find only crashes and failed checks, not memory errors and undefined behavior")
endif ()

set(DXFCXX_FUZZ_TARGETS
        candle_attrs
        candle_symbol
        cmd_args
        market_event_symbols
        order_fields
        order_source
        string_utils
)

foreach (DXFCXX_FUZZ_TARGET ${DXFCXX_FUZZ_TARGETS})
    set(DXFCXX_FUZZ_EXECUTABLE fuzz_${DXFCXX_FUZZ_TARGET})

    add_executable(${DXFCXX_FUZZ_EXECUTABLE} ${DXFCXX_FUZZ_EXECUTABLE}.cpp)
    target_link_libraries(${DXFCXX_FUZZ_EXECUTABLE} PRIVATE dxfcxx::static dxfcxx_warnings)
    target_compile_options(${DXFCXX_FUZZ_EXECUTABLE} PRIVATE -fsanitize=fuzzer)
    target_link_options(${DXFCXX_FUZZ_EXECUTABLE} PRIVATE -fsanitize=fuzzer)
    set_target_properties(${DXFCXX_FUZZ_EXECUTABLE} PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

    add_custom_command(TARGET ${DXFCXX_FUZZ_EXECUTABLE} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dxfcxx::graal>
            $<TARGET_FILE_DIR:${DXFCXX_FUZZ_EXECUTABLE}>)
endforeach ()
