// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz CandleSymbol parsing (pure C++ port of com.dxfeed.event.candle.CandleSymbol).
// Invariants: no crash / UB; only dxfcpp/std exceptions; valueOf(valueOf(s).toString()) == valueOf(s)
// (normalization must be idempotent, as in Java).
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    const std::string s(reinterpret_cast<const char *>(data), size);

    try {
        const auto cs = dxfcpp::CandleSymbol::valueOf(s);
        const std::string str = cs.toString();
        (void)cs.getBaseSymbol();
        (void)cs.getExchange();
        (void)cs.getPrice();
        (void)cs.getSession();
        (void)cs.getPeriod();
        (void)cs.getAlignment();
        (void)cs.getPriceLevel();

        const auto cs2 = dxfcpp::CandleSymbol::valueOf(str);

        if (!(cs == cs2) || cs2.toString() != str) {
            std::fprintf(stderr, "ROUND-TRIP MISMATCH\n in : '%s'\n 1st: '%s'\n 2nd: '%s'\n", s.c_str(), str.c_str(),
                         cs2.toString().c_str());
            __builtin_trap();
        }
    } catch (const std::exception &) {
    }

    return 0;
}
