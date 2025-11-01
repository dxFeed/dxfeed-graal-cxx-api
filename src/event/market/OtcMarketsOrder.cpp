// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OtcMarketsOrder.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &OtcMarketsOrder::TYPE = EventTypeEnum::OTC_MARKETS_ORDER;

void OtcMarketsOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

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

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    graalOtcMarketsOrder->order_base.order_base.market_event.event_type.clazz =
        DXFG_EVENT_OTC_MARKETS_ORDER;
    graalOtcMarketsOrder->quote_access_payment = otcMarketsOrderData_.quoteAccessPayment;
    graalOtcMarketsOrder->otc_markets_flags = otcMarketsOrderData_.otcMarketsFlags;
}

OtcMarketsOrder::Ptr OtcMarketsOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create OtcMarketsOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to create OtcMarketsOrder. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_OTC_MARKETS_ORDER)));
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
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOtcMarketsOrder = new dxfg_otc_markets_order_t{};

    fillGraalData(graalOtcMarketsOrder);

    return graalOtcMarketsOrder;
}

void OtcMarketsOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to free OtcMarketsOrder's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_OTC_MARKETS_ORDER)));
    }

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOtcMarketsOrder;
}

void OtcMarketsOrder::assign(std::shared_ptr<EventType> event) {
    Order::assign(event);

    if (const auto other = event->sharedAs<OtcMarketsOrder>(); other) {
        otcMarketsOrderData_ = other->otcMarketsOrderData_;
    }
}

DXFCPP_END_NAMESPACE