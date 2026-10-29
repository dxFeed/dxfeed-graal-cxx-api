// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/ErrorManager.hpp"
#include "./Isolate.hpp"

#include <exception>
#include <memory>
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

/**
 * Runs a release path that must not throw: a destructor, a deleter, a function that frees native data.
 *
 * An exception of the body is reported to ErrorManager. From the start of the shutdown of the library
 * (dxfcpp::shutdown(), the exit of the process) an exception is expected, since the SDK is being closed or is closed
 * already, and is not reported.
 *
 * @param component The class that releases, for ErrorInfo::component.
 * @param body The release.
 * @param location The source location of the release, for ErrorInfo::location.
 */
template <typename Body>
void runRelease(const char *component, Body &&body,
                const std::source_location &location = std::source_location::current()) noexcept {
    try {
        std::forward<Body>(body)();
    } catch (...) {
        if (!Isolate::isShuttingDown()) {
            ErrorManager::reportException(std::string_view{component}, std::current_exception(), location);
        }
    }
}

/**
 * Releases an object that holds a native listener outside of the callbacks of the library.
 *
 * Releasing the last handle of a native listener waits for its calls in progress on the other threads. Inside a
 * listener or a callback of the library (ListenerCallScope::isActive()) such a wait could deadlock: two listeners that
 * close the subscriptions of each other on two threads would wait for each other. So there the object is passed to the
 * release thread of the library, which is in no callback; elsewhere it is released at once. From the start of the
 * shutdown of the library it is released at once.
 *
 * @param object The object to release.
 */
DXFCPP_EXPORT void releaseOutsideCallbacks(std::shared_ptr<void> object) noexcept;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
