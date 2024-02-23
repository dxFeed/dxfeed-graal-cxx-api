// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace dxfcpp {

struct DXFCPP_EXPORT JavaException : public std::runtime_error {
    static void throwIfJavaThreadExceptionExists();

    std::string stackTrace;

    JavaException(const std::string &message, const std::string &className, std::string stackTrace)
        : std::runtime_error("Java exception of type '" + className + "' was thrown. " + message),
          stackTrace(std::move(stackTrace)) {
    }
};

} // namespace dxfcpp