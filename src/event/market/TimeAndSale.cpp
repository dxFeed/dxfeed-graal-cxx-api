// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

void TimeAndSale::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

const EventTypeEnum &TimeAndSale::Type = EventTypeEnum::TIME_AND_SALE;

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_TIME_AND_SALE) {
        return {};
    }

    try {
        auto graalTimeAndSale = bit_cast<dxfg_time_and_sale_t *>(graalNative);
        auto timeAndSale = std::make_shared<TimeAndSale>(dxfcpp::toString(graalTimeAndSale->market_event.event_symbol));

        timeAndSale->setEventTime(graalTimeAndSale->market_event.event_time);
        timeAndSale->data_ = {
            graalTimeAndSale->event_flags,
            graalTimeAndSale->index,
            graalTimeAndSale->time_nano_part,
            graalTimeAndSale->exchange_code,
            graalTimeAndSale->price,
            graalTimeAndSale->size,
            graalTimeAndSale->bid_price,
            graalTimeAndSale->ask_price,
            dxfcpp::toString(graalTimeAndSale->exchange_sale_conditions),
            graalTimeAndSale->flags,
            dxfcpp::toString(graalTimeAndSale->buyer),
            dxfcpp::toString(graalTimeAndSale->seller),
        };

        return timeAndSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

} // namespace dxfcpp