// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Sanitizer canary: calls a deliberately defective library function. CTest passes the test only if the output contains
// the report of the expected sanitizer (see tests/CMakeLists.txt). Usage: SanitizerCanaryTest <asan|ubsan|tsan>

#include "../../src/internal/SanitizerCanary.hpp"

#include <climits>
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    namespace canary = dxfcpp::internal::sanitizer_canary;

    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <asan|ubsan|tsan>\n", argv[0]);

        return 2;
    }

    volatile int max = INT_MAX;
    int result = 0;

    if (std::strcmp(argv[1], "asan") == 0) {
        result = canary::heapBufferOverflow();
    } else if (std::strcmp(argv[1], "ubsan") == 0) {
        result = canary::signedOverflow(max);
    } else if (std::strcmp(argv[1], "tsan") == 0) {
        result = canary::dataRace();
    } else {
        std::fprintf(stderr, "unknown canary: %s\n", argv[1]);

        return 2;
    }

    // Reached only when the sanitizer did not stop the process.
    std::printf("canary '%s' was NOT detected (result %d)\n", argv[1], result);

    return 0;
}
