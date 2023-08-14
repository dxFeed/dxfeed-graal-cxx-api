// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const SessionType SessionType::NO_TRADING{"NO_TRADING", false};
const SessionType SessionType::PRE_MARKET{"PRE_MARKET", true};
const SessionType SessionType::REGULAR{"REGULAR", true};
const SessionType SessionType::AFTER_MARKET{"AFTER_MARKET", true};

} // namespace dxfcpp