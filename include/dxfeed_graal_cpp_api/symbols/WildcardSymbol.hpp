// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

struct WildcardSymbol final {
    static const WildcardSymbol ALL;

  private:
    std::string symbol;

    WildcardSymbol(const std::string &symbol) noexcept : symbol{symbol} {}

  public:
    WildcardSymbol(const WildcardSymbol &wildcardSymbol) noexcept = default;
    WildcardSymbol(WildcardSymbol &&) noexcept = delete;
    WildcardSymbol &operator=(const WildcardSymbol &wildcardSymbol) noexcept = default;
    WildcardSymbol &operator=(WildcardSymbol &&) noexcept = delete;
    WildcardSymbol() noexcept = default;
    ~WildcardSymbol() noexcept = default;

    const std::string &getSymbol() const noexcept { return symbol; }

    void *toGraal() const noexcept;

    std::string toString() const noexcept { return "WildcardSymbol{" + getSymbol() + "}"; }
};

inline namespace literals {

inline WildcardSymbol operator""_ws(const char *string, size_t length) noexcept { return WildcardSymbol::ALL; }

inline WildcardSymbol operator""_wcs(const char *string, size_t length) noexcept { return WildcardSymbol::ALL; }

} // namespace literals

} // namespace dxfcpp