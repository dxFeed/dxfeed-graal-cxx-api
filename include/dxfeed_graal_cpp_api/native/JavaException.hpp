// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <stdexcept>
#include <string>
#include <utility>

namespace dxfcpp {

struct DXFCPP_EXPORT JavaException : public std::runtime_error {
    static void throwIfJavaThreadExceptionExists();

    std::string stackTrace;

    JavaException(const std::string &message, const std::string &className, std::string stackTrace);
};

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()