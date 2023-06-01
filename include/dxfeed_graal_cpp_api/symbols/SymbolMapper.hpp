// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp {

struct DXFCPP_EXPORT SymbolMapper {
    template <typename SymbolIt>
    static void* toNativeList(SymbolIt begin, SymbolIt end) noexcept;
};

}