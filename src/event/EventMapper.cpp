// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalList(void *graalNativeList) noexcept {
    auto list = static_cast<dxfg_event_type_list *>(graalNativeList);

    if (list->size <= 0) {
        return {};
    }

    std::vector<std::shared_ptr<EventType>> result{};
    result.reserve(static_cast<std::size_t>(list->size));

    for (std::size_t i = 0; i < static_cast<std::size_t>(list->size); i++) {
        auto *e = list->elements[i];

        // TODO: implement other types
        // TODO: type traits
        switch (e->clazz) {
        case DXFG_EVENT_QUOTE:
            result.emplace_back(Quote::fromGraal(e));

            break;
        case DXFG_EVENT_PROFILE:
            result.emplace_back(Profile::fromGraal(e));

            break;
        case DXFG_EVENT_SUMMARY:
            result.emplace_back(Summary::fromGraal(e));

            break;
        case DXFG_EVENT_GREEKS:
            result.emplace_back(Greeks::fromGraal(e));

            break;
        case DXFG_EVENT_CANDLE:
            result.emplace_back(Candle::fromGraal(e));

            break;
        case DXFG_EVENT_DAILY_CANDLE:
            break;
        case DXFG_EVENT_UNDERLYING:
            result.emplace_back(Underlying::fromGraal(e));

            break;
        case DXFG_EVENT_THEO_PRICE:
            result.emplace_back(TheoPrice::fromGraal(e));

            break;
        case DXFG_EVENT_TRADE:
            result.emplace_back(Trade::fromGraal(e));

            break;
        case DXFG_EVENT_TRADE_ETH:
            result.emplace_back(TradeETH::fromGraal(e));

            break;
        case DXFG_EVENT_CONFIGURATION:
            break;
        case DXFG_EVENT_MESSAGE:
            break;
        case DXFG_EVENT_TIME_AND_SALE:
            result.emplace_back(TimeAndSale::fromGraal(e));

            break;
        case DXFG_EVENT_ORDER_BASE:
            break;
        case DXFG_EVENT_ORDER:
            result.emplace_back(Order::fromGraal(e));

            break;
        case DXFG_EVENT_ANALYTIC_ORDER:
            result.emplace_back(AnalyticOrder::fromGraal(e));

            break;
        case DXFG_EVENT_SPREAD_ORDER:
            result.emplace_back(SpreadOrder::fromGraal(e));

            break;
        case DXFG_EVENT_SERIES:
            result.emplace_back(Series::fromGraal(e));

            break;
        case DXFG_EVENT_OPTION_SALE:
            result.emplace_back(OptionSale::fromGraal(e));

            break;
        }
    }

    result.shrink_to_fit();

    return result;
}

void EventMapper::freeGraalList(void *graalList) noexcept {
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
            if (list->elements[elementIndex]) {
                auto *e = list->elements[elementIndex];

                // TODO: implement other types [EN-8235] [EN-8236]
                // TODO: type traits
                switch (e->clazz) {
                case DXFG_EVENT_QUOTE:
                    Quote::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_PROFILE:
                    Profile::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_SUMMARY:
                    Summary::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_GREEKS:
                    Greeks::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_CANDLE:
                    Candle::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_DAILY_CANDLE:
                    break;
                case DXFG_EVENT_UNDERLYING:
                    Underlying::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_THEO_PRICE:
                    TheoPrice::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_TRADE:
                    Trade::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_TRADE_ETH:
                    TradeETH::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_CONFIGURATION:
                    break;
                case DXFG_EVENT_MESSAGE:
                    break;
                case DXFG_EVENT_TIME_AND_SALE:
                    TimeAndSale::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_ORDER_BASE:
                    break;
                case DXFG_EVENT_ORDER:
                    Order::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_ANALYTIC_ORDER:
                    AnalyticOrder::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_SPREAD_ORDER:
                    SpreadOrder::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_SERIES:
                    Series::freeGraal(static_cast<void *>(e));

                    break;
                case DXFG_EVENT_OPTION_SALE:
                    OptionSale::freeGraal(static_cast<void *>(e));

                    break;
                }
            }
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
    } else if (initSize > std::numeric_limits<SizeType>::max()) {
        return std::numeric_limits<SizeType>::max();
    }

    return initSize;
}

void *EventMapper::newGraalList(std::ptrdiff_t size) noexcept {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_event_type_t;

    auto *list = new (std::nothrow) ListType{static_cast<SizeType>(size), nullptr};

    if (!list) {
        // TODO: error handling [EN-8232]
        return nullptr;
    }

    if (size == 0) {
        return static_cast<void *>(list);
    }

    list->elements = new (std::nothrow) ElementType *[size] {
        nullptr
    };

    if (!list->elements) {
        // TODO: error handling [EN-8232]
        delete list;

        return nullptr;
    }

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

bool EventMapper::freeGraalListElements(void *graalList, std::ptrdiff_t count) noexcept {
    using ListType = dxfg_event_type_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr || count < 0 || count >= std::numeric_limits<SizeType>::max()) {
        return false;
    }

    auto *list = static_cast<ListType *>(graalList);

    for (SizeType i = 0; i < count; i++) {
        // TODO: error handling [EN-8232]
        if (list->elements[i]) {
            auto *e = list->elements[i];

            // TODO: implement other types [EN-8235] [EN-8236]
            // TODO: type traits
            switch (e->clazz) {
            case DXFG_EVENT_QUOTE:
                Quote::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_PROFILE:
                Profile::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_SUMMARY:
                Summary::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_GREEKS:
                Greeks::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_CANDLE:
                Candle::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_DAILY_CANDLE:
                break;
            case DXFG_EVENT_UNDERLYING:
                Underlying::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_THEO_PRICE:
                TheoPrice::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_TRADE:
                Trade::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_TRADE_ETH:
                TradeETH::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_CONFIGURATION:
                break;
            case DXFG_EVENT_MESSAGE:
                break;
            case DXFG_EVENT_TIME_AND_SALE:
                TimeAndSale::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_ORDER_BASE:
                break;
            case DXFG_EVENT_ORDER:
                Order::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_ANALYTIC_ORDER:
                AnalyticOrder::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_SPREAD_ORDER:
                SpreadOrder::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_SERIES:
                Series::freeGraal(static_cast<void *>(e));

                break;
            case DXFG_EVENT_OPTION_SALE:
                OptionSale::freeGraal(static_cast<void *>(e));

                break;
            }
        }
    }

    delete[] list->elements;
    delete list;

    return true;
}

DXFCPP_END_NAMESPACE