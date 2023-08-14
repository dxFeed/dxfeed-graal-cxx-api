// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const SessionFilter SessionFilter::ANY{std::nullopt, std::nullopt};
const SessionFilter SessionFilter::TRADING{std::nullopt, true};
const SessionFilter SessionFilter::NON_TRADING{std::nullopt, false};

const SessionFilter SessionFilter::NO_TRADING{SessionType::NO_TRADING, std::nullopt};
const SessionFilter SessionFilter::PRE_MARKET{SessionType::PRE_MARKET, std::nullopt};
const SessionFilter SessionFilter::REGULAR{SessionType::REGULAR, std::nullopt};
const SessionFilter SessionFilter::AFTER_MARKET{SessionType::AFTER_MARKET, std::nullopt};

} // namespace dxfcpp