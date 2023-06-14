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

class DXFCPP_EXPORT IndexedEventSubscriptionSymbol final {
    std::unique_ptr<SymbolWrapper> eventSymbol_;
    IndexedEventSource source_;

  public:
    IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol, const IndexedEventSource &source) noexcept;
    IndexedEventSubscriptionSymbol(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol &
    operator=(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol &operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept;
    IndexedEventSubscriptionSymbol() noexcept = default;
    virtual ~IndexedEventSubscriptionSymbol() noexcept = default;

    const std::unique_ptr<SymbolWrapper> &getEventSymbol() const;

    const IndexedEventSource &getSource() const;

    void *toGraal() const noexcept;

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