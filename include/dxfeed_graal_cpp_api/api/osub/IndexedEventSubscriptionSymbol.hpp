// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../event/IndexedEventSource.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

class IndexedEventSource;
struct SymbolWrapper;

/**
 * Represents subscription to a specific source of indexed events.
 * This is symbol is observed by ObservableSubscriptionChangeListener
 * methods @ref ObservableSubscriptionChangeListener::symbolsAdded() "symbolsAdded"
 * and @ref ObservableSubscriptionChangeListener::symbolsRemoved() "symbolsRemoved"
 * when subscription to IndexedEvent is defined.
 *
 * <p>Instances of this class can be used with DXFeedSubscription to specify subscription
 * to a particular source of indexed events. By default, when subscribing to indexed events by
 * their event symbol object, the subscription is performed to all supported sources.
 *
 * <h3>Equality and hash codes</h3>
 *
 * Indexed event subscription symbols are compared based on their @ref ::getEventSymbol() "eventSymbol" and
 * @ref ::getSource() "source".
 */
class DXFCPP_EXPORT IndexedEventSubscriptionSymbol final {
    std::unique_ptr<SymbolWrapper> eventSymbol_;
    IndexedEventSource source_;

  public:
    /**
     * Creates indexed event subscription symbol with a specified event symbol and source.
     *
     * @param eventSymbol the wrapped event symbol (CandleSymbol, WildcardSymbol, etc).
     * @param source the source.
     */
    IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol, const IndexedEventSource &source) noexcept;

    IndexedEventSubscriptionSymbol(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol &
    operator=(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol &operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol() noexcept = default;
    virtual ~IndexedEventSubscriptionSymbol() noexcept = default;

    /**
     * Returns the wrapped event symbol (CandleSymbol, WildcardSymbol, etc).
     *
     * @return the wrapped event symbol.
     */
    const std::unique_ptr<SymbolWrapper> &getEventSymbol() const;

    /**
     * Returns indexed event source.
     *
     * @return indexed event source.
     */
    const IndexedEventSource &getSource() const;

    void *toGraal() const noexcept;

    static void freeGraal(void* graal) noexcept;

    static IndexedEventSubscriptionSymbol fromGraal(void* graal) noexcept;

    /**
     * Returns string representation of this indexed event subscription symbol.
     *
     * @return string representation of this indexed event subscription symbol.
     */
    std::string toString() const noexcept;

    bool operator==(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept;

    bool operator<(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept;
};

} // namespace dxfcpp

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::IndexedEventSubscriptionSymbol> {
    std::size_t
    operator()(const dxfcpp::IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
        return std::hash<std::unique_ptr<dxfcpp::SymbolWrapper>>{}(indexedEventSubscriptionSymbol.getEventSymbol()) +
               std::hash<dxfcpp::IndexedEventSource>{}(indexedEventSubscriptionSymbol.getSource()) * 31;
    }
};