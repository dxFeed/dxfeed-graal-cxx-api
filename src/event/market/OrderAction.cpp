// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0


#include "../../../include/dxfeed_graal_cpp_api/event/market/OrderAction.hpp"

#include <array>
#include <functional>
#include <span>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

constinit const OrderAction OrderAction::UNDEFINED{0, "UNDEFINED"};
constinit const OrderAction OrderAction::NEW{1, "NEW"};
constinit const OrderAction OrderAction::REPLACE{2, "REPLACE"};
constinit const OrderAction OrderAction::MODIFY{3, "MODIFY"};
constinit const OrderAction OrderAction::DELETE{4, "DELETE"};
constinit const OrderAction OrderAction::PARTIAL{5, "PARTIAL"};
constinit const OrderAction OrderAction::EXECUTE{6, "EXECUTE"};
constinit const OrderAction OrderAction::TRADE{7, "TRADE"};
constinit const OrderAction OrderAction::BUST{8, "BUST"};

namespace {

constinit const std::array<std::reference_wrapper<const OrderAction>, 9> ALL_ELEMENTS{
    std::cref(OrderAction::UNDEFINED), std::cref(OrderAction::NEW),    std::cref(OrderAction::REPLACE),
    std::cref(OrderAction::MODIFY),    std::cref(OrderAction::DELETE), std::cref(OrderAction::PARTIAL),
    std::cref(OrderAction::EXECUTE),   std::cref(OrderAction::TRADE),  std::cref(OrderAction::BUST),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const OrderAction>> OrderAction::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE