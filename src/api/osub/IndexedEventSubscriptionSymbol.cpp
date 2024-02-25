// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol,
                                                               const IndexedEventSource &source) noexcept
    : eventSymbol_(std::make_unique<SymbolWrapper>(eventSymbol)), source_(source) {
}

const std::unique_ptr<SymbolWrapper> &IndexedEventSubscriptionSymbol::getEventSymbol() const {
    return eventSymbol_;
}

const IndexedEventSource &IndexedEventSubscriptionSymbol::getSource() const {
    return source_;
}

void *IndexedEventSubscriptionSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("IndexedEventSubscriptionSymbol::toGraal()");
    }

    auto *graalSymbol = new (std::nothrow) dxfg_indexed_event_subscription_symbol_t{
        .supper = {.type = dxfg_symbol_type_t::INDEXED_EVENT_SUBSCRIPTION},
        .symbol = static_cast<dxfg_symbol_t *>(eventSymbol_->toGraal()),
        .source = static_cast<dxfg_indexed_event_source_t *>(source_.toGraal())};

    return static_cast<void *>(graalSymbol);
}

void IndexedEventSubscriptionSymbol::freeGraal(void *graalNative) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("IndexedEventSubscriptionSymbol::freeGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    auto *graalSymbol = static_cast<dxfg_indexed_event_subscription_symbol_t *>(graalNative);

    SymbolWrapper::freeGraal(graalSymbol->symbol);
    IndexedEventSource::freeGraal(graalSymbol->source);

    delete graalSymbol;
}

IndexedEventSubscriptionSymbol IndexedEventSubscriptionSymbol::fromGraal(void *graalNative) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("IndexedEventSubscriptionSymbol::fromGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return {};
    }

    auto *graalSymbol = static_cast<dxfg_indexed_event_subscription_symbol_t *>(graalNative);

    return {SymbolWrapper::fromGraal(graalSymbol->symbol), IndexedEventSource::fromGraal(graalSymbol->source)};
}

std::string IndexedEventSubscriptionSymbol::toString() const noexcept {
    if constexpr (Debugger::isDebug) {
        return "IndexedEventSubscriptionSymbol{" + eventSymbol_->toString() + ", source = " + source_.toString() + "}";
    } else {
        return eventSymbol_->toString() + "{source=" + source_.toString() + "}";
    }
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
    if (this == &indexedEventSubscriptionSymbol) {
        return *this;
    }

    eventSymbol_ = std::make_unique<SymbolWrapper>(*indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;

    return *this;
}

IndexedEventSubscriptionSymbol &
IndexedEventSubscriptionSymbol::operator=(IndexedEventSubscriptionSymbol &&indexedEventSubscriptionSymbol) noexcept {
    if (this == &indexedEventSubscriptionSymbol) {
        return *this;
    }

    eventSymbol_ = std::move(indexedEventSubscriptionSymbol.eventSymbol_);
    source_ = indexedEventSubscriptionSymbol.source_;

    return *this;
}

} // namespace dxfcpp