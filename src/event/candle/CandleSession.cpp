// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const CandleSession CandleSession::ANY{SessionFilter::ANY, "false"};
const CandleSession CandleSession::REGULAR{SessionFilter::REGULAR, "true"};
const CandleSession CandleSession::DEFAULT = ANY;
const std::string CandleSession::ATTRIBUTE_KEY = "tho";

const std::unordered_map<std::string, std::reference_wrapper<const CandleSession>> CandleSession::BY_STRING_{
    {CandleSession::ANY.toString(), std::cref(CandleSession::ANY)},
    {CandleSession::REGULAR.toString(), std::cref(CandleSession::REGULAR)},
};
} // namespace dxfcpp