// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Direction.hpp"

#include "../../../include/dxfeed_graal_c_api/api.h"
#include <array>
#include <functional>
#include <span>

#include <utility>

DXFCPP_BEGIN_NAMESPACE

constinit const Direction Direction::UNDEFINED{0, "UNDEFINED"};
constinit const Direction Direction::DOWN{1, "DOWN"};
constinit const Direction Direction::ZERO_DOWN{2, "ZERO_DOWN"};
constinit const Direction Direction::ZERO{3, "ZERO"};
constinit const Direction Direction::ZERO_UP{4, "ZERO_UP"};
constinit const Direction Direction::UP{5, "UP"};

namespace {

constinit const std::array<std::reference_wrapper<const Direction>, 6> ALL_ELEMENTS{
    std::cref(Direction::UNDEFINED), std::cref(Direction::DOWN),    std::cref(Direction::ZERO_DOWN),
    std::cref(Direction::ZERO),      std::cref(Direction::ZERO_UP), std::cref(Direction::UP),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const Direction>> Direction::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE