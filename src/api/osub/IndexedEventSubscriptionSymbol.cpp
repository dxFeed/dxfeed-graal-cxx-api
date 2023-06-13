// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include "dxfeed_graal_cpp_api/api/osub/IndexedEventSubscriptionSymbol.hpp"

namespace dxfcpp {

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(const dxfcpp::SymbolWrapper &eventSymbol,
                                                               const dxfcpp::IndexedEventSource &source)
    : eventSymbol_(eventSymbol), source_(source) {}

const SymbolWrapper &IndexedEventSubscriptionSymbol::getEventSymbol() const { return eventSymbol_; }

const IndexedEventSource &IndexedEventSubscriptionSymbol::getSource() const { return source_; }

//TODO: Implement
void *IndexedEventSubscriptionSymbol::toGraal() const noexcept { return nullptr; }

std::string IndexedEventSubscriptionSymbol::toString() const noexcept {
    return eventSymbol_.toString() + "{source=" + source_.toString() + "}";
}

bool IndexedEventSubscriptionSymbol::operator==(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) {
    return eventSymbol_ == indexedEventSubscriptionSymbol.eventSymbol_ &&
           source_ == indexedEventSubscriptionSymbol.source_;
}

bool IndexedEventSubscriptionSymbol::operator<(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) {
    return eventSymbol_ < indexedEventSubscriptionSymbol.eventSymbol_ ||
           (eventSymbol_ == indexedEventSubscriptionSymbol.eventSymbol_ &&
            source_ < indexedEventSubscriptionSymbol.source_);
}

} // namespace dxfcpp