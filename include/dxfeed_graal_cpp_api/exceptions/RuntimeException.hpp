// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/Common.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include <stdexcept>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * A runtime axception with stacktrace
 */
struct DXFCPP_EXPORT RuntimeException : std::runtime_error {
    /**
     * Constructs a runtime exception.
     *
     * @param message The exception's message.
     * @param additionalStackTrace The additional stacktrace that will be concatenated with current stacktrace.
     */
    explicit RuntimeException(const StringLikeWrapper& message, const StringLikeWrapper& additionalStackTrace = "");

    /**
     * @return The current stacktrace + the additional stacktrace if present.
     */
    const std::string& getStackTrace() const &;
private:
    std::string stackTrace_;
};

DXFCPP_END_NAMESPACE

DXFCPP_EXPORT std::ostream &operator<<(std::ostream &os, const dxfcpp::RuntimeException &e);

DXFCXX_DISABLE_MSC_WARNINGS_POP()