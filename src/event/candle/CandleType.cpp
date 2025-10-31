// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleType.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

// The CandleType static fields are initialized in CandlePeriod.cpp

CandleType::CandleType(const StringLike &name, const StringLike &string, std::int64_t periodIntervalMillis) noexcept
    : name_{name}, string_{string}, periodIntervalMillis_{periodIntervalMillis} {
}

std::int64_t CandleType::getPeriodIntervalMillis() const noexcept {
    return periodIntervalMillis_;
}

const std::string &CandleType::getName() const & noexcept {
    return name_;
}

const std::string &CandleType::toString() const & noexcept {
    return string_;
}

std::reference_wrapper<const CandleType> CandleType::parse(const StringLike &s) {
    if (s.empty()) {
        throw InvalidArgumentException("Missing candle type");
    }

    if (const auto result = BY_STRING.find(s); result != BY_STRING.end()) {
        return result->second;
    }

    for (const auto &typeRef : VALUES) {
        const auto &name = typeRef.get().getName();

        // Tick|TICK|tick, Minute|MINUTE|minute, Second|SECOND|second, etc
        if (name.length() >= s.length() && iEquals(name.substr(0, s.length()), s)) {
            return typeRef;
        }

        // Ticks, Minutes, Seconds, etc.
        if (s.ends_with("s") && iEquals(name, s.substr(0, s.length() - 1))) {
            return typeRef;
        }
    }

    throw InvalidArgumentException("Unknown candle type: " + s);
}

bool CandleType::operator==(const CandleType &candleType) const noexcept {
    return string_ == candleType.string_;
}

DXFCPP_END_NAMESPACE