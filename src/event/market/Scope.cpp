// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

const Scope Scope::COMPOSITE{0, "COMPOSITE"};
const Scope Scope::REGIONAL{1, "REGIONAL"};
const Scope Scope::AGGREGATE{2, "AGGREGATE"};
const Scope Scope::ORDER{3, "ORDER"};

template <>
const std::unordered_map<Scope::CodeType, std::reference_wrapper<const Scope>> Scope::ParentType::ALL{
    {Scope::COMPOSITE.getCode(), std::cref(Scope::COMPOSITE)},
    {Scope::REGIONAL.getCode(), std::cref(Scope::REGIONAL)},
    {Scope::AGGREGATE.getCode(), std::cref(Scope::AGGREGATE)},
    {Scope::ORDER.getCode(), std::cref(Scope::ORDER)},
};

DXFCPP_END_NAMESPACE