// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

InvalidArgumentException::InvalidArgumentException(const StringLikeWrapper &message,
                                                   const StringLikeWrapper &additionalStackTrace)
    : RuntimeException(message, additionalStackTrace) {
}

InvalidArgumentException::InvalidArgumentException(const InvalidArgumentException& other) noexcept = default;

DXFCPP_END_NAMESPACE