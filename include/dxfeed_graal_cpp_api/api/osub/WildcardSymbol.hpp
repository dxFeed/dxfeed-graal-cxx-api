// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * Represents [wildcard] subscription to all events of the specific event type.
 * The @ref WildcardSymbol::ALL constant can be added to any DXFeedSubscription instance with
 * @ref DXFeedSubscription::addSymbols() "addSymbols" method to the effect of subscribing to all possible event symbols.
 * The corresponding subscription will start receiving all published events of the corresponding types.
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
     * the impact before using it. It can be used for low-frequency events only (like Forex quotes), because each
     * instance of DXFeedSubscription processes events in a single thread and there is no provision to load-balance
     * wildcard subscription amongst multiple threads. Contact your data provider for the corresponding configuration
     * arrangement if needed.
     *
     * @see WildcardSymbol
     */
    static const WildcardSymbol ALL;

  private:
    std::string symbol_;

    WildcardSymbol(const std::string &symbol) noexcept : symbol_{symbol} {
    }

  public:
    WildcardSymbol(const WildcardSymbol &) noexcept = default;
    WildcardSymbol(WildcardSymbol &&) noexcept = default;
    WildcardSymbol &operator=(const WildcardSymbol &) noexcept = default;
    WildcardSymbol &operator=(WildcardSymbol &&) noexcept = default;
    WildcardSymbol() noexcept = default;
    ~WildcardSymbol() noexcept = default;

    const std::string &getSymbol() const noexcept {
        return symbol_;
    }

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const noexcept;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static const WildcardSymbol &fromGraal(void *graalNative);

    /**
     * Returns string representation of this wildcard subscription symbol.
     *
     * @return string representation of this wildcard subscription symbol.
     */
    std::string toString() const {
        if constexpr (Debugger::isDebug) {
            return "WildcardSymbol{" + symbol_ + "}";
        } else {
            return symbol_;
        }
    }

    bool operator==(const WildcardSymbol &wildcardSymbol) const {
        return symbol_ == wildcardSymbol.symbol_;
    }

    bool operator<(const WildcardSymbol &wildcardSymbol) const {
        return symbol_ < wildcardSymbol.symbol_;
    }
};

inline namespace literals {

inline WildcardSymbol operator""_ws(const char *, size_t) noexcept {
    return WildcardSymbol::ALL;
}

inline WildcardSymbol operator""_wcs(const char *, size_t) noexcept {
    return WildcardSymbol::ALL;
}

} // namespace literals

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::WildcardSymbol> {
    std::size_t operator()(const dxfcpp::WildcardSymbol &wildcardSymbol) const noexcept {
        return std::hash<std::string>{}(wildcardSymbol.getSymbol());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()