// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp {

struct SymbolMapper {
    template <typename SymbolIt>
    static void* toNativeList(SymbolIt begin, SymbolIt end) noexcept;
};

}