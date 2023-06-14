// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include "dxfeed_graal_cpp_api/api/osub/IndexedEventSubscriptionSymbol.hpp"

namespace dxfcpp {

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol,
                                                               const IndexedEventSource &source) noexcept
    : eventSymbol_(std::make_unique<SymbolWrapper>(eventSymbol)), source_(source) {}

const std::unique_ptr<SymbolWrapper> &IndexedEventSubscriptionSymbol::getEventSymbol() const { return eventSymbol_; }

const IndexedEventSource &IndexedEventSubscriptionSymbol::getSource() const { return source_; }

// TODO: Implement
void *IndexedEventSubscriptionSymbol::toGraal() const noexcept { return nullptr; }

std::string IndexedEventSubscriptionSymbol::toString() const noexcept {
    return eventSymbol_->toString() + "{source=" + source_.toString() + "}";
}

bool IndexedEventSubscriptionSymbol::operator==(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
    return *eventSymbol_ == *indexedEventSubscriptionSymbol.eventSymbol_ &&
           source_ == indexedEventSubscriptionSymbol.source_;
}

bool IndexedEventSubscriptionSymbol::operator<(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
    return *eventSymbol_ < *indexedEventSubscriptionSymbol.eventSymbol_ ||
           (*eventSymbol_ == *indexedEventSubscriptionSymbol.eventSymbol_ &&
            source_ < indexedEventSubscriptionSymbol.source_);
}
IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept {
    eventSymbol_ = std::make_unique<SymbolWrapper>(*indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;
}

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(
    IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept {
    eventSymbol_ = std::move(indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;
}

IndexedEventSubscriptionSymbol &IndexedEventSubscriptionSymbol::operator=(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept {
    eventSymbol_ = std::make_unique<SymbolWrapper>(*indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;

    return *this;
}

IndexedEventSubscriptionSymbol &
IndexedEventSubscriptionSymbol::operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept {
    eventSymbol_ = std::move(indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;

    return *this;
}

} // namespace dxfcpp