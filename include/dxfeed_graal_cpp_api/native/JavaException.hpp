// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include <stdexcept>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT JavaException : public std::runtime_error {
    static void throwIfJavaThreadExceptionExists();

    std::string stackTrace;

    JavaException(const std::string &message, const std::string &className, std::string stackTrace);
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()