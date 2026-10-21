// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleType.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

// The CandleType static fields are initialized in CandleSymbol.cpp

CandleType::CandleType() noexcept {
}

CandleType::~CandleType() noexcept {
}

std::int64_t CandleType::getPeriodIntervalMillis() const noexcept {
    return periodIntervalMillis_;
}

std::string CandleType::getName() const {
    return std::string{name_};
}

std::string CandleType::toString() const {
    return std::string{string_};
}

std::reference_wrapper<const CandleType> CandleType::parse(const StringLike &s) {
    if (s.empty()) {
        throw InvalidArgumentException("Missing candle type");
    }

    for (const auto &typeRef : VALUES) {
        if (typeRef.get().string_ == std::string_view{s}) {
            return typeRef;
        }
    }

    for (const auto &typeRef : VALUES) {
        const auto name = typeRef.get().getName();

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