// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include "StringSymbol.hpp"

namespace dxfcpp {

struct StringSymbol;

struct WildcardSymbol final {
    static const WildcardSymbol ALL;

  private:
    std::string symbol{};

    WildcardSymbol(std::string symbol) noexcept: symbol{std::move(symbol)} {}

  public:
    const std::string &getSymbol() const noexcept;

    void *toGraal() const noexcept;

    std::string toString() const noexcept { return getSymbol(); }

    explicit operator std::string() const noexcept { return toString(); }
};

inline namespace literals {

inline WildcardSymbol operator""_ws(const char *string, size_t length) noexcept { return WildcardSymbol::ALL; }

inline WildcardSymbol operator""_wcs(const char *string, size_t length) noexcept { return WildcardSymbol::ALL; }

} // namespace literals

} // namespace dxfcpp