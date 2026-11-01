// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// This header is not part of the API. It is installed only because public headers need it; its contents may change
// without notice.

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/ErrorManager.hpp"

#include <exception>
#include <source_location>
#include <string_view>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace detail {

/**
 * Tells runRelease() whether an exception of a release is to be reported: it is not from the start of the shutdown of
 * the library, nor after a failed creation of the isolate.
 *
 * @return `true` if the exception is to be reported to ErrorManager.
 */
DXFCPP_EXPORT bool isReleaseExceptionReported() noexcept;

} // namespace detail

/**
 * Runs a release path that must not throw: a destructor, a deleter, a function that frees native data.
 *
 * An exception of the body is reported to ErrorManager. From the start of the shutdown of the library
 * (dxfcpp::shutdown(), the exit of the process) an exception is expected, since the SDK is being closed or is closed
 * already, and is not reported. It is not reported after a failed creation of the isolate either.
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
        // After a failed creation of the isolate the objects cannot be released by Java either, as after the shutdown.
        if (detail::isReleaseExceptionReported()) {
            ErrorManager::reportException(std::string_view{component}, std::current_exception(), location);
        }
    }
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
