// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Compiled into the library only in sanitized builds (see cmake/Sanitizers.cmake). Every function below is defective on
// purpose; see SanitizerCanary.hpp.

#include "SanitizerCanary.hpp"

#include <thread>

DXFCPP_BEGIN_NAMESPACE

namespace internal::sanitizer_canary {

// External linkage and volatile accesses keep the defects from being optimized away.
int racyCounter = 0;

int heapBufferOverflow() {
    int *volatile array = new int[4]{};
    array[4] = 1; // one past the end
    const int result = array[0];
    delete[] array;

    return result;
}

int signedOverflow(int value) {
    return value + 1; // UB for INT_MAX
}

int dataRace() {
    std::thread writer([] {
        ++racyCounter;
    });

    ++racyCounter; // concurrent with the write in `writer`
    writer.join();

    return racyCounter;
}

} // namespace internal::sanitizer_canary

DXFCPP_END_NAMESPACE
