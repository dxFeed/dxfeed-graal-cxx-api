// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <functional>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const OrderAction OrderAction::UNDEFINED{0, "UNDEFINED"};
const OrderAction OrderAction::NEW{1, "NEW"};
const OrderAction OrderAction::REPLACE{2, "REPLACE"};
const OrderAction OrderAction::MODIFY{3, "MODIFY"};
const OrderAction OrderAction::DELETE{4, "DELETE"};
const OrderAction OrderAction::PARTIAL{5, "PARTIAL"};
const OrderAction OrderAction::EXECUTE{6, "EXECUTE"};
const OrderAction OrderAction::TRADE{7, "TRADE"};
const OrderAction OrderAction::BUST{8, "BUST"};

template <>
const std::unordered_map<OrderAction::CodeType, std::reference_wrapper<const OrderAction>> OrderAction::ParentType::ALL{
    {OrderAction::UNDEFINED.getCode(), std::cref(OrderAction::UNDEFINED)},
    {OrderAction::NEW.getCode(), std::cref(OrderAction::NEW)},
    {OrderAction::REPLACE.getCode(), std::cref(OrderAction::REPLACE)},
    {OrderAction::MODIFY.getCode(), std::cref(OrderAction::MODIFY)},
    {OrderAction::DELETE.getCode(), std::cref(OrderAction::DELETE)},
    {OrderAction::PARTIAL.getCode(), std::cref(OrderAction::PARTIAL)},
    {OrderAction::EXECUTE.getCode(), std::cref(OrderAction::EXECUTE)},
    {OrderAction::TRADE.getCode(), std::cref(OrderAction::TRADE)},
    {OrderAction::BUST.getCode(), std::cref(OrderAction::BUST)},
};

DXFCPP_END_NAMESPACE