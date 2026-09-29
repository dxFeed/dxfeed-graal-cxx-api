// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz MarketEventSymbols (pure C++ port of com.dxfeed.event.market.MarketEventSymbols).
// Input layout: [exchangeCode:1][symbol]\0[key]\0[value]\0[newBase]
// All functions are declared noexcept -> any exception = std::terminate = crash.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <string>
#include <vector>

static std::vector<std::string> split0(const std::string &s) {
    std::vector<std::string> r(1);
    for (char c : s) {
        if (c == '\0' && r.size() < 4) {
            r.emplace_back();
        } else {
            r.back().push_back(c);
        }
    }
    r.resize(4);
    return r;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    if (size < 1) {
        return 0;
    }

    using M = dxfcpp::MarketEventSymbols;
    const char exchange = static_cast<char>(data[0]);
    const auto parts = split0(std::string(reinterpret_cast<const char *>(data) + 1, size - 1));
    const auto &symbol = parts[0], &key = parts[1], &value = parts[2], &newBase = parts[3];

    (void)M::hasExchangeCode(symbol);
    (void)M::getExchangeCode(symbol);
    const auto changedEx = M::changeExchangeCode(symbol, exchange);
    (void)M::getBaseSymbol(symbol);
    (void)M::changeBaseSymbol(symbol, newBase);
    (void)M::getAttributeStringByKey(symbol, key);
    const auto withAttr = M::changeAttributeStringByKey(symbol, key, value);
    (void)M::removeAttributeStringByKey(symbol, key);

    // Java semantics: after setting key=value (valid key, non-empty), getAttribute must return value.
    (void)M::getAttributeStringByKey(withAttr, key);
    (void)M::getExchangeCode(changedEx);
    (void)M::removeAttributeStringByKey(withAttr, key);

    return 0;
}
