// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <boost/stacktrace.hpp>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

InvalidArgumentException::InvalidArgumentException(const StringLikeWrapper &message,
                                                   const StringLikeWrapper &additionalStackTrace)
    : RuntimeException(message, additionalStackTrace) {
}

DXFCPP_END_NAMESPACE