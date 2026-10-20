// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/utils/StringUtils.hpp"

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * \addtogroup dxfcpp_exceptions
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * A runtime axception with stacktrace
 */
struct DXFCPP_EXPORT RuntimeException : std::runtime_error {
    /**
     * Constructs a runtime exception.
     *
     * @param message The exception's message.
     * @param additionalStackTrace The additional stacktrace that will be concatenated with the current stacktrace.
     */
    explicit RuntimeException(const StringLike &message, const StringLike &additionalStackTrace = "");

    RuntimeException(const RuntimeException &other) noexcept;

    RuntimeException &operator=(const RuntimeException &other) = default;

    /**
     * Returns the stacktrace. The frames are captured when the exception is created and are formatted on the first
     * call.
     *
     * @return The additional stacktrace if present + the current stacktrace.
     */
    const std::string &getStackTrace() const &;

    private:
    struct StackTrace;

    // Shared by the copies, so a copy is cheap and the stacktrace is formatted once.
    std::shared_ptr<StackTrace> stackTrace_;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCPP_EXPORT std::ostream &operator<<(std::ostream &os, const dxfcpp::RuntimeException &e);

DXFCXX_DISABLE_MSC_WARNINGS_POP()