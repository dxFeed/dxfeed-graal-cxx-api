// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz individual candle attribute parsers: CandlePeriod, CandlePriceLevel, CandleType, CandlePrice,
// CandleSession, CandleAlignment. Invariants: no UB (float-cast-overflow in getPeriodIntervalMillis!),
// toString() of a parsed value parses back to the same string and value (CandlePeriod with a finite value,
// CandlePriceLevel).
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

namespace {

// Parses `str` (the toString() of a value parsed from `in`) with `reparse` and traps unless it parses back to `str`.
template <typename Reparse>
std::string checkReparse(const char *what, const std::string &in, const std::string &str, Reparse reparse) {
    std::string again{};

    try {
        again = reparse(str);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "%s toString DOES NOT PARSE in='%s' toString='%s' error='%s'\n", what, in.c_str(),
                     str.c_str(), e.what());
        __builtin_trap();
    }

    if (again != str) {
        std::fprintf(stderr, "%s ROUND-TRIP MISMATCH in='%s' toString='%s' reparsed='%s'\n", what, in.c_str(),
                     str.c_str(), again.c_str());
        __builtin_trap();
    }

    return again;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    using namespace dxfcpp;
    const std::string s(reinterpret_cast<const char *>(data), size);

    try {
        const auto p = CandlePeriod::parse(s);
        (void)p.getPeriodIntervalMillis();

        // As in Java, NaN and infinite values print as "NaNm" or "Infinitym", which do not parse back.
        if (std::isfinite(p.getValue())) {
            const auto reparsed = checkReparse("CandlePeriod", s, p.toString(), [](const std::string &str) {
                return CandlePeriod::parse(str).toString();
            });

            // toString() keeps the exact value (Double.toString round-trips); only the sign of zero may be lost.
            if (!math::exactlyEquals(CandlePeriod::parse(reparsed).getValue(), p.getValue())) {
                std::fprintf(stderr, "CandlePeriod VALUE MISMATCH in='%s' toString='%s'\n", s.c_str(),
                             reparsed.c_str());
                __builtin_trap();
            }
        }
    } catch (const std::exception &) {
    }

    try {
        const auto pl = CandlePriceLevel::parse(s);
        const auto reparsed = checkReparse("CandlePriceLevel", s, pl.toString(), [](const std::string &str) {
            return CandlePriceLevel::parse(str).toString();
        });

        if (!(CandlePriceLevel::parse(reparsed) == pl)) {
            std::fprintf(stderr, "CandlePriceLevel ROUND-TRIP MISMATCH in='%s' toString='%s'\n", s.c_str(),
                         reparsed.c_str());
            __builtin_trap();
        }
    } catch (const std::exception &) {
    }

    try {
        const auto &t = CandleType::parse(s).get();
        (void)t.getPeriodIntervalMillis();
        (void)CandleType::parse(t.toString());
    } catch (const std::exception &) {
    }

    try {
        (void)CandlePrice::parse(s);
    } catch (const std::exception &) {
    }

    try {
        (void)CandleAlignment::parse(s);
    } catch (const std::exception &) {
    }

    return 0;
}
