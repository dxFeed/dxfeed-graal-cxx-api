// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/core.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<EventType> EventMapper::fromGraal(void *graalNativeEvent) {
    if (!graalNativeEvent) {
        throw InvalidArgumentException("EventMapper::fromGraal: The `graalNativeEvent` is nullptr");
    }

    // TODO: implement other types [EN-8235]
    switch (auto *e = dxfcpp::bit_cast<dxfg_event_type_t *>(graalNativeEvent); e->clazz) {
    case DXFG_EVENT_QUOTE:
        return Quote::fromGraal(e);
    case DXFG_EVENT_PROFILE:
        return Profile::fromGraal(e);
    case DXFG_EVENT_SUMMARY:
        return Summary::fromGraal(e);
    case DXFG_EVENT_GREEKS:
        return Greeks::fromGraal(e);
    case DXFG_EVENT_CANDLE:
        return Candle::fromGraal(e);
    case DXFG_EVENT_DAILY_CANDLE:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));
    case DXFG_EVENT_UNDERLYING:
        return Underlying::fromGraal(e);
    case DXFG_EVENT_THEO_PRICE:
        return TheoPrice::fromGraal(e);
    case DXFG_EVENT_TRADE:
        return Trade::fromGraal(e);
    case DXFG_EVENT_TRADE_ETH:
        return TradeETH::fromGraal(e);
    case DXFG_EVENT_CONFIGURATION:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));
    case DXFG_EVENT_MESSAGE:
        return Message::fromGraal(e);
    case DXFG_EVENT_TIME_AND_SALE:
        return TimeAndSale::fromGraal(e);
    case DXFG_EVENT_ORDER_BASE:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));
    case DXFG_EVENT_ORDER:
        return Order::fromGraal(e);
    case DXFG_EVENT_ANALYTIC_ORDER:
        return AnalyticOrder::fromGraal(e);
    case DXFG_EVENT_OTC_MARKETS_ORDER:
        return OtcMarketsOrder::fromGraal(e);
    case DXFG_EVENT_SPREAD_ORDER:
        return SpreadOrder::fromGraal(e);
    case DXFG_EVENT_SERIES:
        return Series::fromGraal(e);
    case DXFG_EVENT_OPTION_SALE:
        return OptionSale::fromGraal(e);
    default:
        throw InvalidArgumentException("Unknown event type: " + std::to_string(static_cast<int>(e->clazz)));
    }
}

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalList(void *graalNativeList) {
    auto list = static_cast<dxfg_event_type_list *>(graalNativeList);

    if (list->size <= 0) {
        return {};
    }

    std::vector<std::shared_ptr<EventType>> result{};
    result.reserve(static_cast<std::size_t>(list->size));

    for (std::size_t i = 0; i < static_cast<std::size_t>(list->size); i++) {
        result.emplace_back(fromGraal(list->elements[i]));
    }

    result.shrink_to_fit();

    return result;
}

void EventMapper::freeGraal(void *graalNativeEvent) {
    if (!graalNativeEvent) {
        return;
    }

    // TODO: implement other types [EN-8235]
    switch (auto *e = dxfcpp::bit_cast<dxfg_event_type_t *>(graalNativeEvent); e->clazz) {
    case DXFG_EVENT_QUOTE:
        Quote::freeGraal(e);

        break;
    case DXFG_EVENT_PROFILE:
        Profile::freeGraal(e);

        break;
    case DXFG_EVENT_SUMMARY:
        Summary::freeGraal(e);

        break;
    case DXFG_EVENT_GREEKS:
        Greeks::freeGraal(e);

        break;
    case DXFG_EVENT_CANDLE:
        Candle::freeGraal(e);

        break;
    case DXFG_EVENT_DAILY_CANDLE:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));

    case DXFG_EVENT_UNDERLYING:
        Underlying::freeGraal(e);

        break;
    case DXFG_EVENT_THEO_PRICE:
        TheoPrice::freeGraal(e);

        break;
    case DXFG_EVENT_TRADE:
        Trade::freeGraal(e);

        break;
    case DXFG_EVENT_TRADE_ETH:
        TradeETH::freeGraal(e);

        break;
    case DXFG_EVENT_CONFIGURATION:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));

    case DXFG_EVENT_MESSAGE:
        Message::freeGraal(e);

        break;
    case DXFG_EVENT_TIME_AND_SALE:
        TimeAndSale::freeGraal(e);

        break;
    case DXFG_EVENT_ORDER_BASE:
        throw InvalidArgumentException("Not emplemented event type: " + std::to_string(static_cast<int>(e->clazz)));

    case DXFG_EVENT_ORDER:
        Order::freeGraal(e);

        break;
    case DXFG_EVENT_ANALYTIC_ORDER:
        AnalyticOrder::freeGraal(e);

        break;
    case DXFG_EVENT_OTC_MARKETS_ORDER:
        OtcMarketsOrder::freeGraal(e);

        break;
    case DXFG_EVENT_SPREAD_ORDER:
        SpreadOrder::freeGraal(e);

        break;
    case DXFG_EVENT_SERIES:
        Series::freeGraal(e);

        break;
    case DXFG_EVENT_OPTION_SALE:
        OptionSale::freeGraal(e);

        break;
    default:
        throw InvalidArgumentException("Unknown event type: " + std::to_string(static_cast<int>(e->clazz)));
    }
}

void EventMapper::freeGraalList(void *graalList) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("EventMapper::freeGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr) {
        return;
    }

    auto list = static_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            freeGraal(list->elements[elementIndex]);
        }

        delete[] list->elements;
    }

    delete list;
}

std::ptrdiff_t EventMapper::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);

    if (initSize < 0) {
        return 0;
    }

    if (initSize > std::numeric_limits<SizeType>::max()) {
        return std::numeric_limits<SizeType>::max();
    }

    return initSize;
}

void *EventMapper::newGraalList(std::ptrdiff_t size) {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_event_type_t;

    auto *list = new ListType{static_cast<SizeType>(size), nullptr};

    if (size == 0) {
        return static_cast<void *>(list);
    }

    list->elements = new ElementType *[size] {
        nullptr
    };

    return list;
}

bool EventMapper::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_event_type_t;

    if (graalList == nullptr || elementIdx < 0 || elementIdx >= std::numeric_limits<SizeType>::max() ||
        element == nullptr) {
        return false;
    }

    static_cast<ListType *>(graalList)->elements[elementIdx] = static_cast<ElementType *>(element);

    return true;
}

bool EventMapper::freeGraalListElements(void *graalList, std::ptrdiff_t count) {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr || count < 0 || count >= std::numeric_limits<SizeType>::max()) {
        return false;
    }

    auto *list = static_cast<ListType *>(graalList);

    for (SizeType i = 0; i < count; i++) {
        freeGraal(list->elements[i]);
    }

    delete[] list->elements;
    delete list;

    return true;
}

DXFCPP_END_NAMESPACE