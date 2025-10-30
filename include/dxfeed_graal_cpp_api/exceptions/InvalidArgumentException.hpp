// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/Common.hpp"
#include "./RuntimeException.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * Thrown to indicate that a method has been passed an illegal or inappropriate argument.
 */
struct DXFCPP_EXPORT InvalidArgumentException : RuntimeException {
    explicit InvalidArgumentException(const StringLike &message,
                                      const StringLike &additionalStackTrace = "");

    InvalidArgumentException(const InvalidArgumentException &other) noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()