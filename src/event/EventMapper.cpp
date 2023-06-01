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

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalNativeList(void *graalNativeList) {
    auto list = bit_cast<dxfg_event_type_list *>(graalNativeList);

    if (list->size <= 0) {
        return {};
    }

    std::vector<std::shared_ptr<EventType>> result{static_cast<std::size_t>(list->size)};

    for (std::size_t i = 0; i < static_cast<std::size_t>(list->size); i++) {
        auto *e = list->elements[i];

        // TODO: implement other types
        switch (e->clazz) {
        case DXFG_EVENT_QUOTE:
            result[i] = Quote::fromGraalNative(e);

            break;
        case DXFG_EVENT_PROFILE:
            result[i] = Profile::fromGraalNative(e);

            break;
        case DXFG_EVENT_SUMMARY:
            result[i] = Summary::fromGraalNative(e);

            break;
        case DXFG_EVENT_GREEKS:
            result[i] = Greeks::fromGraalNative(e);

            break;
        case DXFG_EVENT_CANDLE:
            break;
        case DXFG_EVENT_DAILY_CANDLE:
            break;
        case DXFG_EVENT_UNDERLYING:
            result[i] = Underlying::fromGraalNative(e);

            break;
        case DXFG_EVENT_THEO_PRICE:
            result[i] = TheoPrice::fromGraalNative(e);

            break;
        case DXFG_EVENT_TRADE:
            result[i] = Trade::fromGraalNative(e);

            break;
        case DXFG_EVENT_TRADE_ETH:
            result[i] = TradeETH::fromGraalNative(e);

            break;
        case DXFG_EVENT_CONFIGURATION:
            break;
        case DXFG_EVENT_MESSAGE:
            break;
        case DXFG_EVENT_TIME_AND_SALE:
            result[i] = TimeAndSale::fromGraalNative(e);

            break;
        case DXFG_EVENT_ORDER_BASE:
            break;
        case DXFG_EVENT_ORDER:
            break;
        case DXFG_EVENT_ANALYTIC_ORDER:
            break;
        case DXFG_EVENT_SPREAD_ORDER:
            break;
        case DXFG_EVENT_SERIES:
            result[i] = Series::fromGraalNative(e);

            break;
        case DXFG_EVENT_OPTION_SALE:
            result[i] = OptionSale::fromGraalNative(e);

            break;
        }
    }

    return result;
}

} // namespace dxfcpp