// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/schedule/SessionType.hpp"
#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

const SessionType SessionType::NO_TRADING{SessionTypeEnum::NO_TRADING, "NO_TRADING", false};
const SessionType SessionType::PRE_MARKET{SessionTypeEnum::PRE_MARKET, "PRE_MARKET", true};
const SessionType SessionType::REGULAR{SessionTypeEnum::REGULAR, "REGULAR", true};
const SessionType SessionType::AFTER_MARKET{SessionTypeEnum::AFTER_MARKET, "AFTER_MARKET", true};

SessionFilter::SessionFilter(SessionFilterEnum code, std::string name, std::optional<SessionType> type,
                             std::optional<bool> trading) noexcept
    : code_{code}, name_{std::move(name)}, type_{type}, trading_{trading} {
}

const JavaObjectHandle<SessionFilter> &SessionFilter::getHandle() const & {
    std::lock_guard<std::mutex> lock(mtx_);

    if (!handle_) {
        handle_ = isolated::schedule::IsolatedSessionFilter::getInstance(static_cast<std::uint32_t>(code_));
    }

    return handle_;
}

const SessionFilter SessionFilter::ANY{SessionFilterEnum::ANY, "ANY", std::nullopt, std::nullopt};
const SessionFilter SessionFilter::TRADING{SessionFilterEnum::TRADING, "TRADING", std::nullopt, true};
const SessionFilter SessionFilter::NON_TRADING{SessionFilterEnum::NON_TRADING, "NON_TRADING", std::nullopt, false};

const SessionFilter SessionFilter::NO_TRADING{SessionFilterEnum::NO_TRADING, "NO_TRADING", SessionType::NO_TRADING,
                                              std::nullopt};
const SessionFilter SessionFilter::PRE_MARKET{SessionFilterEnum::PRE_MARKET, "PRE_MARKET", SessionType::PRE_MARKET,
                                              std::nullopt};
const SessionFilter SessionFilter::REGULAR{SessionFilterEnum::REGULAR, "REGULAR", SessionType::REGULAR, std::nullopt};
const SessionFilter SessionFilter::AFTER_MARKET{SessionFilterEnum::AFTER_MARKET, "AFTER_MARKET",
                                                SessionType::AFTER_MARKET, std::nullopt};

const CandleSession CandleSession::ANY{SessionFilter::ANY, "false"};
const CandleSession CandleSession::REGULAR{SessionFilter::REGULAR, "true"};
const CandleSession CandleSession::DEFAULT = ANY;
const std::string CandleSession::ATTRIBUTE_KEY = "tho";

const std::unordered_map<std::string, std::reference_wrapper<const CandleSession>> CandleSession::BY_STRING{
    {CandleSession::ANY.toString(), std::cref(CandleSession::ANY)},
    {CandleSession::REGULAR.toString(), std::cref(CandleSession::REGULAR)},
};

const std::vector<std::reference_wrapper<const CandleSession>> CandleSession::VALUES{
    std::cref(CandleSession::ANY),
    std::cref(CandleSession::REGULAR),
};

DXFCPP_END_NAMESPACE