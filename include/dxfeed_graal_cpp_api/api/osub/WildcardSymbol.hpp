// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

/**
 * Represents [wildcard] subscription to all events of the specific event type.
 * The @ref WildcardSymbol::ALL constant can be added to any DXFeedSubscription instance with @ref
 * DXFeedSubscription::addSymbols() "addSymbols" method to the effect of subscribing to all possible event symbols. The
 * corresponding subscription will start receiving all published events of the corresponding types.
 */
struct DXFCPP_EXPORT WildcardSymbol final {
    /**
     * Symbol prefix that is reserved for wildcard subscriptions.
     * Any subscription starting with "*" is ignored with the exception of WildcardSymbol subscription.
     */
    static const std::string RESERVED_PREFIX;

    /**
     * Represents [wildcard] subscription to all events of the specific event type.
     *
     * <p><b>NOTE:</b> Wildcard subscription can create extremely high network and CPU load for certain kinds of
     * high-frequency events like quotes. It requires a special arrangement on the side of upstream data provider and
     * is disabled by default in upstream feed configuration. Make that sure you have adequate resources and understand
     * the impact before using it. It can be used for low-frequency events only (like Forex quotes), because each instance
     * of DXFeedSubscription processes events in a single thread and there is no provision to load-balance wildcard
     * subscription amongst multiple threads.
     * Contact your data provider for the corresponding configuration arrangement if needed.
     *
     * @see WildcardSymbol
     */
    static const WildcardSymbol ALL;

  private:
    std::string symbol_;

    WildcardSymbol(const std::string &symbol) noexcept : symbol_{symbol} {}

  public:
    WildcardSymbol(const WildcardSymbol &) noexcept = default;
    WildcardSymbol(WildcardSymbol &&) noexcept = default;
    WildcardSymbol &operator=(const WildcardSymbol &) noexcept = default;
    WildcardSymbol &operator=(WildcardSymbol &&) noexcept = default;
    WildcardSymbol() noexcept = default;
    virtual ~WildcardSymbol() noexcept = default;

    const std::string &getSymbol() const noexcept { return symbol_; }

    void *toGraal() const noexcept;

    static void freeGraal(void *graal) noexcept;

    static const WildcardSymbol &fromGraal(void *graal) noexcept;

    /**
     * Returns string representation of this wildcard subscription symbol.
     *
     * @return string representation of this wildcard subscription symbol.
     */
    std::string toString() const noexcept {
        if constexpr (Debugger::isDebug) {
            return "WildcardSymbol{" + symbol_ + "}";
        } else {
            return symbol_;
        }
    }

    bool operator==(const WildcardSymbol &wildcardSymbol) const { return symbol_ == wildcardSymbol.symbol_; }

    bool operator<(const WildcardSymbol &wildcardSymbol) const { return symbol_ < wildcardSymbol.symbol_; }
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