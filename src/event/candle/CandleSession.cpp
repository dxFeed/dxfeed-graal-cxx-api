// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleSession.hpp"

#include "../../../include/dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp"
#include "../../../include/dxfeed_graal_cpp_api/schedule/SessionFilter.hpp"
#include "../../../include/dxfeed_graal_cpp_api/schedule/SessionType.hpp"

#include <mutex>
#include <optional>
#include <string>

#include "../../internal/utils/CandleUtils.hpp"

DXFCPP_BEGIN_NAMESPACE

SessionType::SessionType() noexcept {
}

SessionTypeEnum SessionType::getCode() const noexcept {
    return code_;
}

std::string SessionType::getName() const {
    return std::string{name_};
}

bool SessionType::isTrading() const noexcept {
    return trading_;
}

std::string SessionType::toString() const {
    return std::string{name_};
}

bool SessionType::operator==(const SessionType &sessionType) const noexcept {
    return code_ == sessionType.getCode();
}

SessionFilter::SessionFilter() noexcept {
}

SessionFilter::~SessionFilter() noexcept {
}

SessionFilter::SessionFilter(SessionFilterEnum code, const StringLike &name, std::optional<SessionType> type,
                             std::optional<bool> trading) noexcept
    : code_{code}, name_{ConstString::copyOf(name)}, type_{type}, trading_{trading} {
}

bool SessionFilter::operator==(const SessionFilter &sessionFilter) const noexcept {
    return type_ == sessionFilter.type_ && trading_ == sessionFilter.trading_;
}

std::string SessionFilter::toString() const {
    return std::string("SessionFilter(") + ((!type_) ? "null" : type_.value().toString()) + ", " +
           ((!trading_) ? "null" : dxfcpp::toString(trading_.value())) + ")";
}

const std::optional<SessionType> &SessionFilter::getType() const & noexcept {
    return type_;
}

const std::optional<bool> &SessionFilter::getTrading() const & noexcept {
    return trading_;
}

const JavaObjectHandle<SessionFilter> &SessionFilter::getHandle() const & {
    // Not a member: the constants are constant-initialized, and std::mutex is not constexpr in every configuration.
    static std::mutex mutex{};
    std::lock_guard lock(mutex);

    if (!handle_) {
        handle_ = isolated::schedule::IsolatedSessionFilter::getInstance(static_cast<std::uint32_t>(code_));
    }

    return handle_;
}

CandleSession::CandleSession() noexcept : CandleSession(SessionFilter::ANY, "false") {
}

CandleSession::~CandleSession() noexcept {
}

const SessionFilter &CandleSession::getSessionFilter() const & noexcept {
    return *sessionFilter_;
}

std::string CandleSession::changeAttributeForSymbol(const StringLike &symbol) const {
    return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, candle::SESSION_KEY)
                            : MarketEventSymbols::changeAttributeStringByKey(symbol, candle::SESSION_KEY, toString());
}

std::string CandleSession::toString() const {
    return std::string{string_};
}

bool CandleSession::operator==(const CandleSession &candleSession) const noexcept {
    return *sessionFilter_ == candleSession.getSessionFilter() && string_ == candleSession.string_;
}

std::reference_wrapper<const CandleSession> CandleSession::parse(const StringLike &s) {
    if (s.empty()) {
        throw InvalidArgumentException("Missing candle session");
    }

    for (const auto &sessionRef : VALUES) {
        if (sessionRef.get().string_ == std::string_view{s}) {
            return sessionRef;
        }
    }

    for (const auto &sessionRef : VALUES) {
        if (const auto sessionStr = sessionRef.get().toString();
            sessionStr.length() >= s.length() && iEquals(sessionStr.substr(0, s.length()), s)) {
            return sessionRef;
        }
    }

    throw InvalidArgumentException("Unknown candle session: " + s);
}

std::reference_wrapper<const CandleSession> CandleSession::getAttributeForSymbol(const StringLike &symbol) {
    const auto stringOpt = MarketEventSymbols::getAttributeStringByKey(symbol, candle::SESSION_KEY);

    return stringOpt && iEquals(stringOpt.value(), "true") ? std::cref(REGULAR) : std::cref(DEFAULT);
}

std::string CandleSession::normalizeAttributeForSymbol(const StringLike &symbol) noexcept {
    const auto a = MarketEventSymbols::getAttributeStringByKey(symbol, candle::SESSION_KEY);

    if (!a) {
        return symbol;
    }

    // As getAttributeForSymbol() (Java Boolean.parseBoolean): only "true" in any case is the regular session.
    if (!iEquals(a.value(), "true")) {
        return candle::removeDefaultAttribute(symbol, candle::SESSION_KEY, DEFAULT.toString());
    }

    if (a.value() != REGULAR.toString()) {
        return MarketEventSymbols::changeAttributeStringByKey(symbol, candle::SESSION_KEY, REGULAR.toString());
    }

    return symbol;
}
const std::string &CandleSession::ATTRIBUTE_KEY() {
    static const auto *const value = new std::string{candle::SESSION_KEY};

    return *value;
}

const std::unordered_map<std::string, std::reference_wrapper<const CandleSession>> &CandleSession::BY_STRING() {
    static const auto *const value = new std::unordered_map<std::string, std::reference_wrapper<const CandleSession>>{
        {ANY.toString(), std::cref(ANY)},
        {REGULAR.toString(), std::cref(REGULAR)},
    };

    return *value;
}

DXFCPP_END_NAMESPACE
