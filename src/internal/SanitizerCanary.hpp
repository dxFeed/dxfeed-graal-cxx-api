// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

// Deliberately defective functions compiled into the library only in sanitized builds (DXFCXX_SANITIZERS_ENABLED).
// The sanitizer canary tests call them and expect the sanitizer report: this proves that the sanitizer is active and
// that the LIBRARY code (not only the test code) is instrumented. Not part of the public API; not installed.

#include "../../include/dxfeed_graal_cpp_api/internal/Conf.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace internal::sanitizer_canary {

/// Writes one element past the end of a heap array (AddressSanitizer: heap-buffer-overflow).
DXFCPP_EXPORT int heapBufferOverflow();

/// Returns `value + 1` without an overflow check (UndefinedBehaviorSanitizer: signed integer overflow for INT_MAX).
DXFCPP_EXPORT int signedOverflow(int value);

/// Increments a global counter from two threads without synchronization (ThreadSanitizer: data race).
DXFCPP_EXPORT int dataRace();

} // namespace internal::sanitizer_canary

DXFCPP_END_NAMESPACE
