// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/candle/CandleSession.hpp>
#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp>
#include <dxfeed_graal_cpp_api/schedule/SessionFilter.hpp>
#include <dxfeed_graal_cpp_api/schedule/SessionType.hpp>
#include <mutex>
#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const SessionType SessionType::NO_TRADING{SessionTypeEnum::NO_TRADING, "NO_TRADING", false};
const SessionType SessionType::PRE_MARKET{SessionTypeEnum::PRE_MARKET, "PRE_MARKET", true};
const SessionType SessionType::REGULAR{SessionTypeEnum::REGULAR, "REGULAR", true};
const SessionType SessionType::AFTER_MARKET{SessionTypeEnum::AFTER_MARKET, "AFTER_MARKET", true};

SessionFilter::SessionFilter(SessionFilterEnum code, const StringLike &name, std::optional<SessionType> type,
                             std::optional<bool> trading) noexcept
    : code_{code}, name_{name}, type_{type}, trading_{trading} {
}

const JavaObjectHandle<SessionFilter> &SessionFilter::getHandle() const & {
    std::lock_guard lock(mtx_);

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
    {ANY.toString(), std::cref(ANY)},
    {REGULAR.toString(), std::cref(REGULAR)},
};

const std::vector<std::reference_wrapper<const CandleSession>> CandleSession::VALUES{
    std::cref(ANY),
    std::cref(REGULAR),
};

CandleSession::CandleSession(const SessionFilter &sessionFilter, const dxfcpp::StringLike &string) noexcept
    : sessionFilter_{&sessionFilter}, string_{string} {
}

const SessionFilter &CandleSession::getSessionFilter() const & noexcept {
    return *sessionFilter_;
}

std::string CandleSession::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
}

const std::string &CandleSession::toString() const & noexcept {
    return string_;
}

bool CandleSession::operator==(const CandleSession &candleSession) const noexcept {
    return *sessionFilter_ == candleSession.getSessionFilter() && string_ == candleSession.toString();
}

std::reference_wrapper<const CandleSession> CandleSession::parse(const StringLike &s) {
    if (s.empty()) {
        throw InvalidArgumentException("Missing candle session");
    }

    if (const auto found = BY_STRING.find(s); found != BY_STRING.end()) {
        return found->second;
    }

    for (const auto &sessionRef : VALUES) {
        if (const auto &sessionStr = sessionRef.get().toString();
            sessionStr.length() >= s.length() && iEquals(sessionStr.substr(0, s.length()), s)) {
            return sessionRef;
        }
    }

    throw InvalidArgumentException("Unknown candle session: " + s);
}

std::reference_wrapper<const CandleSession> CandleSession::getAttributeForSymbol(const StringLike &symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    return !stringOpt ? std::cref(DEFAULT) : parse(stringOpt.value());
}

std::string CandleSession::normalizeAttributeForSymbol(const StringLike &symbol) noexcept {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, ATTRIBUTE_KEY);

    if (!a) {
        return symbol;
    }

    try {
        const auto other = parse(a.value());

        if (other.get() == DEFAULT) {
            return MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY);
        }

        if (a.value() != other.get().toString()) {
            return MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, other.get().toString());
        }

        return symbol;
    } catch (const InvalidArgumentException &) {
        return symbol;
    } catch (const std::invalid_argument &) {
        return symbol;
    }
}
DXFCPP_END_NAMESPACE