// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

class IndexedEventSource;
struct SymbolWrapper;

class DXFCPP_EXPORT IndexedEventSubscriptionSymbol final {
    SymbolWrapper eventSymbol_;
    IndexedEventSource source_;

  public:
    IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol, const IndexedEventSource &source);

    const SymbolWrapper &getEventSymbol() const;

    const IndexedEventSource &getSource() const;

    void *toGraal() const noexcept;

    std::string toString() const noexcept;

    bool operator==(const IndexedEventSubscriptionSymbol &wildcardSymbol);

    bool operator<(const IndexedEventSubscriptionSymbol &wildcardSymbol);
};

} // namespace dxfcpp

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::IndexedEventSubscriptionSymbol> {
    std::size_t
    operator()(const dxfcpp::IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
        return std::hash<dxfcpp::SymbolWrapper>{}(indexedEventSubscriptionSymbol.getEventSymbol()) +
               std::hash<dxfcpp::IndexedEventSource>{}(indexedEventSubscriptionSymbol.getSource()) * 31;
    }
};