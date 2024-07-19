// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &OtcMarketsOrder::TYPE = EventTypeEnum::OTC_MARKETS_ORDER;

void OtcMarketsOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    otcMarketsOrderData_ = {
        .quoteAccessPayment = graalOtcMarketsOrder->quote_access_payment,
        .otcMarketsFlags = graalOtcMarketsOrder->otc_markets_flags,
    };
}

void OtcMarketsOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillGraalData(graalNative);

    auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    graalOtcMarketsOrder->order_base.order_base.market_event.event_type.clazz =
        dxfg_event_clazz_t::DXFG_EVENT_OTC_MARKETS_ORDER;
    graalOtcMarketsOrder->quote_access_payment = otcMarketsOrderData_.quoteAccessPayment;
    graalOtcMarketsOrder->otc_markets_flags = otcMarketsOrderData_.otcMarketsFlags;
}

OtcMarketsOrder::Ptr OtcMarketsOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw std::invalid_argument("Unable to create OtcMarketsOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw std::invalid_argument(
            fmt::format("Unable to create OtcMarketsOrder. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_OTC_MARKETS_ORDER))));
    }

    auto otcMarketsOrder = std::make_shared<OtcMarketsOrder>();

    otcMarketsOrder->fillData(graalNative);

    return otcMarketsOrder;
}

std::string OtcMarketsOrder::toString() const {
    return fmt::format("OtcMarketsOrder{{{}, marketMaker={}, QAP={}, open={}, unsolicited={}, priceType={}, "
                       "saturated={}, autoEx={}, NMS={}}}",
                       baseFieldsToString(), getMarketMaker(), getQuoteAccessPayment(), isOpen(), isUnsolicited(),
                       getOtcMarketsPriceType().toString(), isSaturated(), isAutoExecution(), isNmsConditional());
}

void *OtcMarketsOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOtcMarketsOrder = new dxfg_otc_markets_order_t{};

    fillGraalData(static_cast<void *>(graalOtcMarketsOrder));

    return static_cast<void *>(graalOtcMarketsOrder);
}

void OtcMarketsOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw std::invalid_argument(
            fmt::format("Unable to free OtcMarketsOrder's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_OTC_MARKETS_ORDER))));
    }

    auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    Order::freeGraalData(graalNative);

    delete graalOtcMarketsOrder;
}

DXFCPP_END_NAMESPACE