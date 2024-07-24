// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/IndexedEventSource.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <cstdint>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

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
 * Indexed event subscription symbols are compared based on their @ref IndexedEventSubscriptionSymbol::getEventSymbol()
 * "eventSymbol" and
 * @ref IndexedEventSubscriptionSymbol::getSource() "source".
 */
class DXFCPP_EXPORT IndexedEventSubscriptionSymbol {
    friend SymbolWrapper;

    std::unique_ptr<SymbolWrapper> eventSymbol_;
    IndexedEventSource source_;

  protected:
    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    virtual void *toGraal() const;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure (recursively
     * if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static IndexedEventSubscriptionSymbol fromGraal(void *graalNative);

  public:
    /**
     * Creates indexed event subscription symbol with a specified event symbol and source.
     *
     * @param eventSymbol the wrapped event symbol (CandleSymbol, WildcardSymbol, etc).
     * @param source the source.
     */
    IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol, const IndexedEventSource &source);

    IndexedEventSubscriptionSymbol(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol);

    IndexedEventSubscriptionSymbol(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;

    IndexedEventSubscriptionSymbol &
    operator=(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol);

    IndexedEventSubscriptionSymbol &operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol() noexcept = default;
    virtual ~IndexedEventSubscriptionSymbol() noexcept = default;

    /**
     * Returns the wrapped event symbol (CandleSymbol, WildcardSymbol, etc).
     *
     * @return the wrapped event symbol.
     */
    virtual const std::unique_ptr<SymbolWrapper> &getEventSymbol() const;

    /**
     * Returns indexed event source.
     *
     * @return indexed event source.
     */
    virtual const IndexedEventSource &getSource() const;

    /**
     * Returns string representation of this indexed event subscription symbol.
     *
     * @return string representation of this indexed event subscription symbol.
     */
    virtual std::string toString() const;

    bool operator==(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept;

    bool operator<(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept;
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::IndexedEventSubscriptionSymbol> {
    std::size_t
    operator()(const dxfcpp::IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
        return std::hash<std::unique_ptr<dxfcpp::SymbolWrapper>>{}(indexedEventSubscriptionSymbol.getEventSymbol()) +
               std::hash<dxfcpp::IndexedEventSource>{}(indexedEventSubscriptionSymbol.getSource()) * 31;
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()