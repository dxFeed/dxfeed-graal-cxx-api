// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"
#include "dxfeed_graal_cpp_api/api/osub/IndexedEventSubscriptionSymbol.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol,
                                                               const IndexedEventSource &source)
    : eventSymbol_(std::make_unique<SymbolWrapper>(eventSymbol)),
      source_(std::make_unique<IndexedEventSource>(source)) {
}

const std::unique_ptr<SymbolWrapper> &IndexedEventSubscriptionSymbol::getEventSymbol() const {
    return eventSymbol_;
}

const std::unique_ptr<IndexedEventSource> &IndexedEventSubscriptionSymbol::getSource() const {
    return source_;
}

void *IndexedEventSubscriptionSymbol::toGraal() const {
    auto *graalSymbol = new dxfg_indexed_event_subscription_symbol_t{
        .supper = {.type = dxfg_symbol_type_t::INDEXED_EVENT_SUBSCRIPTION},
        .symbol = static_cast<dxfg_symbol_t *>(eventSymbol_->toGraal()),
        .source = static_cast<dxfg_indexed_event_source_t *>(source_->toGraal())};

    return static_cast<void *>(graalSymbol);
}

void IndexedEventSubscriptionSymbol::freeGraal(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    auto *graalSymbol = static_cast<dxfg_indexed_event_subscription_symbol_t *>(graalNative);

    SymbolWrapper::freeGraal(graalSymbol->symbol);
    IndexedEventSource::freeGraal(graalSymbol->source);

    delete graalSymbol;
}

IndexedEventSubscriptionSymbol IndexedEventSubscriptionSymbol::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("IndexedEventSubscriptionSymbol::fromGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        throw InvalidArgumentException(
            "Unable to create IndexedEventSubscriptionSymbol. The `graalNative` parameter is nullptr");
    }

    auto *graalSymbol = static_cast<dxfg_indexed_event_subscription_symbol_t *>(graalNative);

    return {SymbolWrapper::fromGraal(graalSymbol->symbol), IndexedEventSource::fromGraal(graalSymbol->source)};
}

std::string IndexedEventSubscriptionSymbol::toString() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        return "IndexedEventSubscriptionSymbol{" + eventSymbol_->toString() + ", source = " + source_->toString() + "}";
    } else {
        return eventSymbol_->toStringUnderlying() + "{source=" + source_->toString() + "}";
    }
}

bool IndexedEventSubscriptionSymbol::operator==(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
    return toString() == indexedEventSubscriptionSymbol.toString();
}

bool IndexedEventSubscriptionSymbol::operator<(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) const noexcept {
    return toString() < indexedEventSubscriptionSymbol.toString();
}

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(
    const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) {
    eventSymbol_ = std::make_unique<SymbolWrapper>(*indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = std::make_unique<IndexedEventSource>(*indexedEventSubscriptionSymbol.source_);
}

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(
    IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept {
    eventSymbol_ = std::move(indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = std::move(indexedEventSubscriptionSymbol.source_);
}

IndexedEventSubscriptionSymbol &
IndexedEventSubscriptionSymbol::operator=(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) {
    if (this == &indexedEventSubscriptionSymbol) {
        return *this;
    }

    eventSymbol_ = std::make_unique<SymbolWrapper>(*indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = std::make_unique<IndexedEventSource>(*indexedEventSubscriptionSymbol.source_);

    return *this;
}

IndexedEventSubscriptionSymbol &
IndexedEventSubscriptionSymbol::operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept {
    if (this == &indexedEventSubscriptionSymbol) {
        return *this;
    }

    eventSymbol_ = std::move(indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = std::move(indexedEventSubscriptionSymbol.source_);

    return *this;
}

DXFCPP_END_NAMESPACE