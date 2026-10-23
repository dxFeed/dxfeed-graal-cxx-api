// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/custom/NuamTrade.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../internal/utils/JavaUtils.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &NuamTrade::TYPE = EventTypeEnum::NUAM_TRADE;

void NuamTrade::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Trade::fillData(graalNative);

    const auto graalNuamTrade = static_cast<dxfg_nuam_trade_t *>(graalNative);

    nuamTradeData_ = {
        .tradeStatTime = graalNuamTrade->trade_stat_time,
        .lastSignificantPrice = graalNuamTrade->last_significant_price,
        .lastPriceForAll = graalNuamTrade->last_price_for_all,
        .numberOfTrades = graalNuamTrade->number_of_trades,
        .vwap = graalNuamTrade->vwap,
    };
}

void NuamTrade::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Trade::fillGraalData(graalNative);

    const auto graalNuamTrade = static_cast<dxfg_nuam_trade_t *>(graalNative);

    graalNuamTrade->trade_base.trade_base.market_event.event_type.clazz = DXFG_EVENT_NUAM_TRADE;
    graalNuamTrade->trade_stat_time = nuamTradeData_.tradeStatTime;
    graalNuamTrade->last_significant_price = nuamTradeData_.lastSignificantPrice;
    graalNuamTrade->last_price_for_all = nuamTradeData_.lastPriceForAll;
    graalNuamTrade->number_of_trades = nuamTradeData_.numberOfTrades;
    graalNuamTrade->vwap = nuamTradeData_.vwap;
}

NuamTrade::Ptr NuamTrade::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create NuamTrade. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_TRADE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format("Unable to create NuamTrade. Wrong event class {}({})! Expected: {}",
                                                   isolated::toString(eventType), std::to_string(eventType),
                                                   isolated::toString(DXFG_EVENT_NUAM_TRADE)));
    }

    auto nuamTrade = std::make_shared<NuamTrade>();

    nuamTrade->fillData(graalNative);

    return nuamTrade;
}

void *NuamTrade::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalNuamTrade = new dxfg_nuam_trade_t{};

    fillGraalData(graalNuamTrade);

    return graalNuamTrade;
}

void NuamTrade::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_TRADE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free NuamTrade's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_NUAM_TRADE)));
    }

    const auto graalNuamTrade = static_cast<dxfg_nuam_trade_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalNuamTrade;
}

void NuamTrade::assign(std::shared_ptr<EventType> event) {
    Trade::assign(event);

    if (const auto other = event->sharedAs<NuamTrade>(); other) {
        nuamTradeData_ = other->nuamTradeData_;
    }
}

NuamTrade::NuamTrade() noexcept {
}

NuamTrade::NuamTrade(const StringLike &eventSymbol) noexcept : Trade(eventSymbol) {
}

std::int64_t NuamTrade::getTradeStatTime() const noexcept {
    return nuamTradeData_.tradeStatTime;
}

void NuamTrade::setTradeStatTime(std::int64_t tradeStatTime) noexcept {
    nuamTradeData_.tradeStatTime = tradeStatTime;
}

double NuamTrade::getLastSignificantPrice() const noexcept {
    return nuamTradeData_.lastSignificantPrice;
}

void NuamTrade::setLastSignificantPrice(double lastSignificantPrice) noexcept {
    nuamTradeData_.lastSignificantPrice = lastSignificantPrice;
}

double NuamTrade::getLastPriceForAll() const noexcept {
    return nuamTradeData_.lastPriceForAll;
}

void NuamTrade::setLastPriceForAll(double lastPriceForAll) noexcept {
    nuamTradeData_.lastPriceForAll = lastPriceForAll;
}

std::int32_t NuamTrade::getNumberOfTrades() const noexcept {
    return nuamTradeData_.numberOfTrades;
}

void NuamTrade::setNumberOfTrades(std::int32_t numberOfTrades) noexcept {
    nuamTradeData_.numberOfTrades = numberOfTrades;
}

double NuamTrade::getVWAP() const noexcept {
    return nuamTradeData_.vwap;
}

void NuamTrade::setVWAP(double vwap) noexcept {
    nuamTradeData_.vwap = vwap;
}

std::string NuamTrade::toString() const {
    return fmt::format("NuamTrade{{{}, tradeStatTime={}, lastSignificantPrice={}, lastPriceForAll={}, "
                       "numberOfTrades={}, vwap={}}}",
                       baseFieldsToString(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTradeStatTime()),
                       java::doubleToString(getLastSignificantPrice()), java::doubleToString(getLastPriceForAll()),
                       getNumberOfTrades(), java::doubleToString(getVWAP()));
}

DXFCPP_END_NAMESPACE
