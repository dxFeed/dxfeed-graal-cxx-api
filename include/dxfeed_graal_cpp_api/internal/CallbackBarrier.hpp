// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/ErrorManager.hpp"
#include "./Isolate.hpp"

#include <exception>
#include <source_location>
#include <string_view>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * Runs the body of a callback that the dxFeed Graal SDK calls (a C function pointer passed to the SDK).
 *
 * The body is skipped when the library is shutting down (see Isolate::CallbackScope). An exception of the body is
 * reported to ErrorManager: it must not reach the SDK, where it would terminate the process.
 *
 * @param component The part of the library that owns the callback, for ErrorInfo::component.
 * @param body The body of the callback.
 * @param location The source location of the callback, for ErrorInfo::location.
 */
template <typename Body>
void runCallback(const char *component, Body &&body,
                 const std::source_location &location = std::source_location::current()) noexcept {
    const Isolate::CallbackScope callbackScope{};

    if (!callbackScope) {
        return;
    }

    try {
        std::forward<Body>(body)();
    } catch (...) {
        // A view of the name: copying it into a string could throw std::bad_alloc out of this noexcept function.
        ErrorManager::reportException(std::string_view{component}, std::current_exception(), location);
    }
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
