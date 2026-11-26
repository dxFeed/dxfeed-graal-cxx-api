// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"

#include "../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/candle/Candle.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/custom/NuamOrder.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/custom/NuamTimeAndSale.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/custom/NuamTrade.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/AnalyticOrder.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/MarketMaker.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/OptionSale.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Order.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/OrderImbalance.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/OtcMarketsOrder.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Profile.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Quote.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/SpreadOrder.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Summary.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Trade.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/TradeETH.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/misc/Message.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/misc/TextConfiguration.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/misc/TextMessage.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/option/Greeks.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/option/Series.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/option/TheoPrice.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/option/Underlying.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/util/TimeFormat.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"
#include "../internal/utils/debug/Debug.hpp"
#include "../internal/utils/GraalArena.hpp"
#include "../internal/Common.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <dxfg_v4_api.h>
#include <fmt/format.h>
#include <map>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace {

// The class of a native event as an integer: a newer SDK can send a class that dxfg_v4_event_clazz_t does not have, and
// loading such a value as the enum is undefined behaviour.
std::int32_t eventClass(const dxfg_v4_event_type_t *event) noexcept {
    std::int32_t clazz{};

    static_assert(sizeof(clazz) == sizeof(event->clazz));
    std::memcpy(&clazz, &event->clazz, sizeof(clazz));

    return clazz;
}

std::string eventTypeName(std::int32_t clazz) {
    switch (clazz) {
    case DXFG_EVENT_DAILY_CANDLE:
        return "DAILY_CANDLE";
    case DXFG_EVENT_CONFIGURATION:
        return EventTypeEnum::CONFIGURATION.getName();
    case DXFG_EVENT_ORDER_BASE:
        return "ORDER_BASE";
    default:
        break;
    }

    if (const auto it = EventTypeEnum::ALL_BY_ID.find(static_cast<std::uint32_t>(clazz));
        it != EventTypeEnum::ALL_BY_ID.end()) {
        return it->second.get().getName();
    }

    return std::to_string(clazz);
}

} // namespace

std::shared_ptr<EventType> EventMapper::fromGraal(void *graalNativeEvent) {
    if (!graalNativeEvent) {
        return {};
    }

    // TODO: implement other types [EN-8235]
    switch (auto *e = dxfcpp::bit_cast<dxfg_v4_event_type_t *>(graalNativeEvent); eventClass(e)) {
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
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));
    case DXFG_EVENT_UNDERLYING:
        return Underlying::fromGraal(e);
    case DXFG_EVENT_THEO_PRICE:
        return TheoPrice::fromGraal(e);
    case DXFG_EVENT_TRADE:
        return Trade::fromGraal(e);
    case DXFG_EVENT_TRADE_ETH:
        return TradeETH::fromGraal(e);
    case DXFG_EVENT_CONFIGURATION:
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));
    case DXFG_EVENT_MESSAGE:
        return Message::fromGraal(e);
    case DXFG_EVENT_TIME_AND_SALE:
        return TimeAndSale::fromGraal(e);
    case DXFG_EVENT_ORDER_BASE:
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));
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
    case DXFG_EVENT_TEXT_MESSAGE:
        return TextMessage::fromGraal(e);
    case DXFG_EVENT_ORDER_IMBALANCE:
        return OrderImbalance::fromGraal(e);
    case DXFG_EVENT_NUAM_ORDER:
        return NuamOrder::fromGraal(e);
    case DXFG_EVENT_NUAM_TRADE:
        return NuamTrade::fromGraal(e);
    case DXFG_EVENT_NUAM_TIME_AND_SALE:
        return NuamTimeAndSale::fromGraal(e);
    case DXFG_EVENT_MARKET_MAKER:
        return MarketMaker::fromGraal(e);
    case DXFG_EVENT_TEXT_CONFIGURATION:
        return TextConfiguration::fromGraal(e);
    default:
        throw InvalidArgumentException("Unknown event type: " + std::to_string(eventClass(e)));
    }
}

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalListSkippingErrors(void *graalNativeList) {
    const auto list = static_cast<dxfg_v4_event_type_list *>(graalNativeList);

    if (list == nullptr || list->size <= 0) {
        return {};
    }

    struct Skipped {
        std::size_t count{};
        std::string firstError{};
    };

    std::vector<std::shared_ptr<EventType>> result{};
    std::map<std::int32_t, Skipped> skippedByClass{};

    result.reserve(static_cast<std::size_t>(list->size));

    for (std::size_t i = 0; i < static_cast<std::size_t>(list->size); i++) {
        auto *e = dxfcpp::bit_cast<dxfg_v4_event_type_t *>(list->elements[i]);

        if (e == nullptr) {
            continue;
        }

        std::string error{};

        try {
            if (auto event = fromGraal(e)) {
                result.emplace_back(std::move(event));
            }

            continue;
        } catch (const std::exception &ex) {
            error = ex.what();
        } catch (...) {
            error = "unknown exception";
        }

        auto &skipped = skippedByClass[eventClass(e)];

        if (skipped.count++ == 0) {
            skipped.firstError = std::move(error);
        }
    }

    for (const auto &[clazz, skipped] : skippedByClass) {
        ErrorManager::report("EventMapper", fmt::format("{} event(s) of type {} skipped: {}", skipped.count,
                                                        eventTypeName(clazz), skipped.firstError));
    }

    return result;
}

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalList(void *graalNativeList) {
    const auto list = static_cast<dxfg_v4_event_type_list *>(graalNativeList);

    if (list == nullptr || list->size <= 0) {
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
    switch (auto *e = dxfcpp::bit_cast<dxfg_v4_event_type_t *>(graalNativeEvent); eventClass(e)) {
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
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));

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
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));

    case DXFG_EVENT_MESSAGE:
        Message::freeGraal(e);

        break;
    case DXFG_EVENT_TIME_AND_SALE:
        TimeAndSale::freeGraal(e);

        break;
    case DXFG_EVENT_ORDER_BASE:
        throw InvalidArgumentException("Not implemented event type: " + std::to_string(eventClass(e)));

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
    case DXFG_EVENT_TEXT_MESSAGE:
        TextMessage::freeGraal(e);

        break;
    case DXFG_EVENT_ORDER_IMBALANCE:
        OrderImbalance::freeGraal(e);

        break;
    case DXFG_EVENT_NUAM_ORDER:
        NuamOrder::freeGraal(e);

        break;
    case DXFG_EVENT_NUAM_TRADE:
        NuamTrade::freeGraal(e);

        break;
    case DXFG_EVENT_NUAM_TIME_AND_SALE:
        NuamTimeAndSale::freeGraal(e);

        break;
    case DXFG_EVENT_MARKET_MAKER:
        MarketMaker::freeGraal(e);

        break;
    case DXFG_EVENT_TEXT_CONFIGURATION:
        TextConfiguration::freeGraal(e);

        break;
    default:
        throw InvalidArgumentException("Unknown event type: " + std::to_string(eventClass(e)));
    }
}

void EventMapper::freeGraalList(void *graalList) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("EventMapper::freeGraalList(graalList = " + toStringAny(graalList) + ")");
    }

    using ListType = dxfg_v4_event_type_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr) {
        return;
    }

    const auto list = static_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            freeGraal(list->elements[elementIndex]);
        }

        delete[] list->elements;
    }

    delete list;
}

std::ptrdiff_t EventMapper::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    using ListType = dxfg_v4_event_type_list;
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
    using ListType = dxfg_v4_event_type_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_v4_event_type_t;

    auto *list = new ListType{static_cast<SizeType>(size), nullptr};

    if (size == 0) {
        return list;
    }

    list->elements = new ElementType *[static_cast<std::size_t>(size)] {
        nullptr
    };

    return list;
}

bool EventMapper::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept {
    using ListType = dxfg_v4_event_type_list;
    using SizeType = decltype(ListType::size);
    using ElementType = dxfg_v4_event_type_t;

    if (graalList == nullptr || elementIdx < 0 || elementIdx >= std::numeric_limits<SizeType>::max() ||
        element == nullptr) {
        return false;
    }

    static_cast<ListType *>(graalList)->elements[elementIdx] = static_cast<ElementType *>(element);

    return true;
}

bool EventMapper::freeGraalListElements(void *graalList, std::ptrdiff_t count) {
    using ListType = dxfg_v4_event_type_list;
    using SizeType = decltype(ListType::size);

    if (graalList == nullptr || count < 0 || count >= std::numeric_limits<SizeType>::max()) {
        return false;
    }

    const auto *list = static_cast<ListType *>(graalList);

    for (SizeType i = 0; i < count; i++) {
        freeGraal(list->elements[i]);
    }

    delete[] list->elements;
    delete list;

    return true;
}

namespace {

// The list of an arena with the arena before it, so that freeGraalListInArena() finds the arena by the list.
struct ArenaList {
    GraalArena *arena;
    dxfg_v4_event_type_list list;
};

static_assert(std::is_standard_layout_v<ArenaList>);

// The first block holds the list, its array and this many bytes per event: a trade with a short symbol fits.
constexpr std::size_t FIRST_BLOCK_BYTES_PER_EVENT = 160;
constexpr std::size_t FIRST_BLOCK_MIN_CAPACITY = 1024;

ArenaList *arenaListOf(void *graalList) noexcept {
    return reinterpret_cast<ArenaList *>(static_cast<unsigned char *>(graalList) - offsetof(ArenaList, list));
}

// Whether the event is of a class of the library itself: its toGraal() takes all memory from newGraalEvent() and
// createCString(), so it may run in the arena. A subclass outside the library may mix its own allocations with those of
// the library (for example, its own structure with the strings of MarketEvent::fillGraalData()), so it runs without the
// arena. A list mostly holds events of one class: the last class found is checked first.
bool isLibraryEventClass(const EventType &event, const std::type_info *&lastFound) noexcept {
    const auto &type = typeid(event);

    if (&type == lastFound) {
        return true;
    }

    static const std::type_info *const LIBRARY_CLASSES[] = {
        &typeid(Trade),           &typeid(TradeETH),        &typeid(Quote),       &typeid(Profile),
        &typeid(Summary),         &typeid(TimeAndSale),     &typeid(Order),       &typeid(SpreadOrder),
        &typeid(AnalyticOrder),   &typeid(OtcMarketsOrder), &typeid(MarketMaker), &typeid(OptionSale),
        &typeid(OrderImbalance),  &typeid(Candle),          &typeid(Greeks),      &typeid(Series),
        &typeid(TheoPrice),       &typeid(Underlying),      &typeid(Message),     &typeid(TextMessage),
        &typeid(TextConfiguration), &typeid(NuamOrder),     &typeid(NuamTrade),   &typeid(NuamTimeAndSale),
    };

    for (const auto *libraryClass : LIBRARY_CLASSES) {
        if (*libraryClass == type) {
            lastFound = &type;

            return true;
        }
    }

    return false;
}

// The events whose conversion did not take its memory from the arena are freed on their own.
void freeEventsOutsideArena(const GraalArena *arena, dxfg_v4_event_type_t *const *elements,
                            std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; i++) {
        if (elements[i] != nullptr && !arena->owns(elements[i])) {
            try {
                EventMapper::freeGraal(elements[i]);
            } catch (...) {
                // An event of an unknown class: nothing else can be done with it.
            }
        }
    }
}

} // namespace

void *EventMapper::newGraalListInArena(std::ptrdiff_t size, NextEvent next, void *state) {
    using SizeType = decltype(dxfg_v4_event_type_list::size);
    using ElementType = dxfg_v4_event_type_t;

    const auto count = static_cast<std::size_t>(std::max<std::ptrdiff_t>(size, 0));
    const auto capacity = std::clamp(sizeof(ArenaList) + count * (sizeof(ElementType *) + FIRST_BLOCK_BYTES_PER_EVENT),
                                     FIRST_BLOCK_MIN_CAPACITY, GraalArena::BLOCK_CAPACITY);
    auto *arena = GraalArena::create(capacity);
    ElementType **elements = nullptr;
    std::size_t converted = 0;

    try {
        auto *arenaList = new (arena->allocate(sizeof(ArenaList), alignof(ArenaList)))
            ArenaList{arena, {static_cast<SizeType>(count), nullptr}};

        // Zero size is needed, for example, to clear the list of symbols.
        if (count == 0) {
            return &arenaList->list;
        }

        elements = static_cast<ElementType **>(arena->allocate(count * sizeof(ElementType *), alignof(ElementType *)));
        std::fill_n(elements, count, nullptr);
        arenaList->list.elements = elements;

        const std::type_info *lastLibraryClass = nullptr;

        for (; converted < count; converted++) {
            const auto *event = next(state);

            if (event == nullptr) {
                throw InvalidArgumentException("Unable to convert the event " + std::to_string(converted) +
                                               " of the list: the event is null");
            }

            ElementType *element = nullptr;

            {
                // Another class converts its event on the heap, even if a scope of another arena is open.
                const GraalArena::Scope scope(isLibraryEventClass(*event, lastLibraryClass) ? arena : nullptr);

                element = static_cast<ElementType *>(event->toGraal());
            }

            if (element == nullptr) {
                throw InvalidArgumentException("Unable to convert the event " + std::to_string(converted) +
                                               " of the list: " + event->toString());
            }

            elements[converted] = element;
        }

        return &arenaList->list;
    } catch (...) {
        if (elements != nullptr) {
            freeEventsOutsideArena(arena, elements, converted);
        }

        GraalArena::destroy(arena);

        throw;
    }
}

void EventMapper::freeGraalListInArena(void *graalList) noexcept {
    if (graalList == nullptr) {
        return;
    }

    const auto *arenaList = arenaListOf(graalList);
    auto *arena = arenaList->arena;

    if (arenaList->list.size > 0 && arenaList->list.elements != nullptr) {
        freeEventsOutsideArena(arena, arenaList->list.elements, static_cast<std::size_t>(arenaList->list.size));
    }

    GraalArena::destroy(arena);
}

DXFCPP_END_NAMESPACE