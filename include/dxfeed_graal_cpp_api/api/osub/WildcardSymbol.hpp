// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

struct DXFCPP_EXPORT WildcardSymbol final {
    static const WildcardSymbol ALL;

  private:
    std::string symbol;

    WildcardSymbol(const std::string &symbol) noexcept : symbol{symbol} {}

  public:
    WildcardSymbol(const WildcardSymbol &) noexcept = default;
    WildcardSymbol(WildcardSymbol &&) noexcept = default;
    WildcardSymbol &operator=(const WildcardSymbol &) noexcept = default;
    WildcardSymbol &operator=(WildcardSymbol &&) noexcept = default;
    WildcardSymbol() noexcept = default;
    ~WildcardSymbol() noexcept = default;

    const std::string &getSymbol() const noexcept { return symbol; }

    void *toGraal() const noexcept;

    std::string toString() const noexcept { return "WildcardSymbol{" + getSymbol() + "}"; }

    bool operator==(const WildcardSymbol &wildcardSymbol) const { return getSymbol() == wildcardSymbol.getSymbol(); }

    auto operator<(const WildcardSymbol &wildcardSymbol) const { return getSymbol() < wildcardSymbol.getSymbol(); }
};

inline namespace literals {

inline WildcardSymbol operator""_ws(const char *, size_t) noexcept { return WildcardSymbol::ALL; }

inline WildcardSymbol operator""_wcs(const char *, size_t) noexcept { return WildcardSymbol::ALL; }

} // namespace literals

} // namespace dxfcpp

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::WildcardSymbol> {
    std::size_t operator()(const dxfcpp::WildcardSymbol &wildcardSymbol) const noexcept {
        return std::hash<std::string>{}(wildcardSymbol.getSymbol());
    }
};