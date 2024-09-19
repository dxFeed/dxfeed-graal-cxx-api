// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/Common.hpp"
#include "RuntimeException.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include <stdexcept>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Thrown to indicate that a method has been passed an illegal or inappropriate argument.
 */
struct DXFCPP_EXPORT InvalidArgumentException : RuntimeException {
    explicit InvalidArgumentException(const StringLikeWrapper &message,
                                      const StringLikeWrapper &additionalStackTrace = "");

    InvalidArgumentException(const InvalidArgumentException& other) noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()