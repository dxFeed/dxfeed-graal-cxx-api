// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleSession.hpp"

#include "../../../include/dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp"
#include "../../../include/dxfeed_graal_cpp_api/schedule/SessionFilter.hpp"
#include "../../../include/dxfeed_graal_cpp_api/schedule/SessionType.hpp"

#include <mutex>
#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

SessionType::SessionType(SessionTypeEnum code, const StringLike &name, bool trading) noexcept
    : code_{code}, name_{name}, trading_{trading} {
}

SessionType::SessionType() noexcept {
}

SessionType::~SessionType() noexcept {
}

SessionTypeEnum SessionType::getCode() const noexcept {
    return code_;
}

const std::string &SessionType::getName() const & noexcept {
    return name_;
}

bool SessionType::isTrading() const noexcept {
    return trading_;
}

const std::string &SessionType::toString() const & noexcept {
    return name_;
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
    : code_{code}, name_{name}, type_{type}, trading_{trading} {
}

const JavaObjectHandle<SessionFilter> &SessionFilter::getHandle() const & {
    std::lock_guard lock(mtx_);

    if (!handle_) {
        handle_ = isolated::schedule::IsolatedSessionFilter::getInstance(static_cast<std::uint32_t>(code_));
    }

    return handle_;
}

CandleSession::CandleSession(const SessionFilter &sessionFilter, const StringLike &string) noexcept
    : sessionFilter_{&sessionFilter}, string_{string} {
}

CandleSession::CandleSession() noexcept {
}

CandleSession::~CandleSession() noexcept {
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