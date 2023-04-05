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

const EventTypeEnum &OptionSale::Type = EventTypeEnum::OPTION_SALE;

std::shared_ptr<OptionSale> OptionSale::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_OPTION_SALE) {
        return {};
    }

    try {
        auto graalOptionSale = bit_cast<dxfg_option_sale_t *>(graalNative);
        auto optionSale = std::make_shared<OptionSale>(dxfcpp::toString(graalOptionSale->market_event.event_symbol));

        optionSale->setEventTime(graalOptionSale->market_event.event_time);
        optionSale->data_ = {
            graalOptionSale->event_flags,
            graalOptionSale->index,
            graalOptionSale->time_sequence,
            graalOptionSale->time_nano_part,
            graalOptionSale->exchange_code,
            graalOptionSale->price,
            graalOptionSale->size,
            graalOptionSale->bid_price,
            graalOptionSale->ask_price,
            dxfcpp::toString(graalOptionSale->exchange_sale_conditions),
            graalOptionSale->flags,
            graalOptionSale->underlying_price,
            graalOptionSale->volatility,
            graalOptionSale->delta,
            dxfcpp::toString(graalOptionSale->option_symbol),
        };

        return optionSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

void OptionSale::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

} // namespace dxfcpp