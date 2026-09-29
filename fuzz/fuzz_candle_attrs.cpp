// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz individual candle attribute parsers: CandlePeriod, CandlePriceLevel, CandleType, CandlePrice,
// CandleSession, CandleAlignment. Invariants: no UB (float-cast-overflow in getPeriodIntervalMillis!),
// parse(x.toString()) == x.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    using namespace dxfcpp;
    const std::string s(reinterpret_cast<const char *>(data), size);

    try {
        const auto p = CandlePeriod::parse(s);
        (void)p.getPeriodIntervalMillis();
        const auto p2 = CandlePeriod::parse(p.toString());
        if (!(p == p2)) {
            std::fprintf(stderr, "CandlePeriod ROUND-TRIP MISMATCH in='%s' toString='%s' reparsed='%s' v1=%g v2=%g\n",
                         s.c_str(), p.toString().c_str(), p2.toString().c_str(), p.getValue(), p2.getValue());
            __builtin_trap();
        }
    } catch (const std::exception &) {
    }

    try {
        const auto pl = CandlePriceLevel::parse(s);
        (void)CandlePriceLevel::parse(pl.toString());
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
