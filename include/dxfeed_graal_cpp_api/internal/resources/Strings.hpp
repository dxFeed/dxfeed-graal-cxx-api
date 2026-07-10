// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace internal::resources {

struct DXFCPP_EXPORT Strings {
    struct DXFCPP_EXPORT Events {
        static constexpr auto UNABLE_TO_CREATE = "Unable to create {}. The `{}` parameter is nullptr";
        static constexpr auto UNABLE_TO_CREATE2 = "Unable to create {}. Wrong event class {}({})! Expected: {}";
        static constexpr auto UNABLE_TO_FREE = "Unable to free {}'s Graal data. Wrong event class {}({})! Expected: {}";
        static constexpr auto INVALID_VALUE_FOR_ARG = "Invalid value for argument `{}`: {}";
        static constexpr auto UNKNOWN_CANDLE_ = "Unknown candle {}: {}";
        static constexpr auto MISSING_CANDLE_ = "Missing candle {}";
    };
};

} // namespace internal::resources

namespace ires = internal::resources;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
