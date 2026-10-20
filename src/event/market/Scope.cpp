// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Scope.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const Scope Scope::COMPOSITE{0, "COMPOSITE"};
constinit const Scope Scope::REGIONAL{1, "REGIONAL"};
constinit const Scope Scope::AGGREGATE{2, "AGGREGATE"};
constinit const Scope Scope::ORDER{3, "ORDER"};

namespace {

constinit const std::array<std::reference_wrapper<const Scope>, 4> ALL_ELEMENTS{
    std::cref(Scope::COMPOSITE),
    std::cref(Scope::REGIONAL),
    std::cref(Scope::AGGREGATE),
    std::cref(Scope::ORDER),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const Scope>> Scope::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE