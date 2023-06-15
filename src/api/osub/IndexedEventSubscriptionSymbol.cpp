// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include "dxfeed_graal_cpp_api/api/osub/IndexedEventSubscriptionSymbol.hpp"

namespace dxfcpp {

// TODO: implement recursive construction\destruction
struct IndexedEventSubscriptionSymbol::Impl final {
    dxfg_indexed_event_subscription_symbol_t graalSymbol;

    Impl() noexcept : graalSymbol{{INDEXED_EVENT_SUBSCRIPTION}, nullptr} {}

    Impl(const Impl &) noexcept {
        graalSymbol.supper = {INDEXED_EVENT_SUBSCRIPTION};
        graalSymbol.symbol = nullptr;
    }

    Impl(Impl &&) noexcept {
        graalSymbol.supper = {INDEXED_EVENT_SUBSCRIPTION};
        graalSymbol.symbol = nullptr;
    }

    Impl &operator=(const Impl &impl) noexcept {
        if (this == &impl) {
            return *this;
        }

        graalSymbol.supper = {INDEXED_EVENT_SUBSCRIPTION};
        graalSymbol.symbol = nullptr;

        return *this;
    }

    Impl &operator=(Impl &&impl) noexcept {
        if (this == &impl) {
            return *this;
        }

        graalSymbol.supper = {INDEXED_EVENT_SUBSCRIPTION};
        graalSymbol.symbol = nullptr;

        return *this;
    }
};

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol(const SymbolWrapper &eventSymbol,
                                                               const IndexedEventSource &source) noexcept
    : eventSymbol_(std::make_unique<SymbolWrapper>(eventSymbol)), source_(source) {}

const std::unique_ptr<SymbolWrapper> &IndexedEventSubscriptionSymbol::getEventSymbol() const { return eventSymbol_; }

const IndexedEventSource &IndexedEventSubscriptionSymbol::getSource() const { return source_; }

void *IndexedEventSubscriptionSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "IndexedEventSubscriptionSymbol::toGraal()");
    }

    auto *graalSymbol = new (std::nothrow)
        dxfg_indexed_event_subscription_symbol_t{{INDEXED_EVENT_SUBSCRIPTION},
                                                 dxfcpp::bit_cast<dxfg_symbol_t *>(eventSymbol_->toGraal()),
                                                 dxfcpp::bit_cast<dxfg_indexed_event_source_t *>(source_.toGraal())};

    return dxfcpp::bit_cast<void *>(graalSymbol);
}

void IndexedEventSubscriptionSymbol::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "IndexedEventSubscriptionSymbol::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_indexed_event_subscription_symbol_t *>(graal);

    SymbolWrapper::freeGraal(graalSymbol->symbol);
    IndexedEventSource::freeGraal(graalSymbol->source);

    delete graalSymbol;
}

IndexedEventSubscriptionSymbol IndexedEventSubscriptionSymbol::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(
            "IndexedEventSubscriptionSymbol::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_indexed_event_subscription_symbol_t *>(graal);

    return {SymbolWrapper::fromGraal(graalSymbol->symbol), IndexedEventSource::fromGraal(graalSymbol->source)};
}

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

IndexedEventSubscriptionSymbol::IndexedEventSubscriptionSymbol() noexcept
    : impl_{std::make_unique<IndexedEventSubscriptionSymbol::Impl>()} {}

IndexedEventSubscriptionSymbol::~IndexedEventSubscriptionSymbol() noexcept = default;

} // namespace dxfcpp