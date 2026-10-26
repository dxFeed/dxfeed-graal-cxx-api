// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/orcs/PriceLevelService.hpp"

#include "../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/orcs/IsolatedPriceLevelService.hpp"

#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

PriceLevelService::PriceLevelService(LockExternalConstructionTag, JavaObjectHandle<PriceLevelService> &&handle)
    : handle_(std::move(handle)) {
}

PriceLevelService::~PriceLevelService() noexcept {
    runRelease("PriceLevelService", [&] {
        close();
    });
}

std::shared_ptr<PriceLevelService> PriceLevelService::create(const StringLike &address) {
    return createShared(isolated::orcs::IsolatedPriceLevelService::create(address));
}

std::vector<std::shared_ptr<Order>> PriceLevelService::getOrders(const CandleSymbol &candleSymbol,
                                                                 const OrderSource &orderSource, std::int64_t from,
                                                                 std::int64_t to, const StringLike &caller) const {
    return convertEvents<EventType, Order>(
        isolated::orcs::IsolatedPriceLevelService::getOrders(handle_, candleSymbol, orderSource, from, to, caller));
}

std::vector<std::shared_ptr<Order>> PriceLevelService::getOrders(const CandleSymbol &candleSymbol,
                                                                 const OrderSource &orderSource, std::int64_t from,
                                                                 std::int64_t to) const {
    return convertEvents<EventType, Order>(
        isolated::orcs::IsolatedPriceLevelService::getOrders(handle_, candleSymbol, orderSource, from, to));
}

std::vector<std::shared_ptr<Quote>> PriceLevelService::getQuotes(const CandleSymbol &candleSymbol, std::int64_t from,
                                                                 std::int64_t to, const StringLike &caller) const {
    return convertEvents<EventType, Quote>(
        isolated::orcs::IsolatedPriceLevelService::getQuotes(handle_, candleSymbol, from, to, caller));
}

std::vector<std::shared_ptr<Quote>> PriceLevelService::getQuotes(const CandleSymbol &candleSymbol, std::int64_t from,
                                                                 std::int64_t to) const {
    return convertEvents<EventType, Quote>(
        isolated::orcs::IsolatedPriceLevelService::getQuotes(handle_, candleSymbol, from, to));
}

std::shared_ptr<AuthOrderSource> PriceLevelService::getAuthOrderSource() const {
    return AuthOrderSource::createShared(isolated::orcs::IsolatedPriceLevelService::getAuthOrderSource(handle_));
}

void PriceLevelService::close() {
    if (closed_.exchange(true)) {
        return;
    }

    try {
        isolated::orcs::IsolatedPriceLevelService::close(handle_);
    } catch (...) {
        // The service is not closed: the next close() or the destructor tries again.
        closed_ = false;

        throw;
    }
}

DXFCPP_END_NAMESPACE
