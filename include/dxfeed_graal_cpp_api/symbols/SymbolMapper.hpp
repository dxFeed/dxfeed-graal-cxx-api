// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "SymbolWrapper.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT SymbolMapper {
    template <typename SymbolIt>
    static void* toGraalList(SymbolIt begin, SymbolIt end) noexcept;

    static void freeGraalList(void *graalList) noexcept;

    static std::vector<SymbolWrapper> fromGraal(void *graalList) noexcept;
};

}