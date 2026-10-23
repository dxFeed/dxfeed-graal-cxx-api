// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"

#include <dxfg_api.h>

#include <array>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

DXFCPP_BEGIN_NAMESPACE

constinit const EventTypeEnum EventTypeEnum::INVALID_EVENT_TYPE{static_cast<std::uint32_t>(-1), "INVALID", "Invalid",
                                                                false};

constinit const EventTypeEnum EventTypeEnum::QUOTE{DXFG_EVENT_QUOTE, "QUOTE", "Quote", true};
constinit const EventTypeEnum EventTypeEnum::PROFILE{DXFG_EVENT_PROFILE, "PROFILE", "Profile", true};
constinit const EventTypeEnum EventTypeEnum::SUMMARY{DXFG_EVENT_SUMMARY, "SUMMARY", "Summary", true};
constinit const EventTypeEnum EventTypeEnum::GREEKS{DXFG_EVENT_GREEKS, "GREEKS", "Greeks", true, true, true};
constinit const EventTypeEnum EventTypeEnum::CANDLE{DXFG_EVENT_CANDLE, "CANDLE", "Candle", true, true, true, false};

// const EventTypeEnum EventTypeEnum::DAILY_CANDLE{DXFG_EVENT_DAILY_CANDLE, "DAILY_CANDLE", "DailyCandle", true, true,
// true, false};

constinit const EventTypeEnum EventTypeEnum::UNDERLYING{
    DXFG_EVENT_UNDERLYING, "UNDERLYING", "Underlying", true, true, true};
constinit const EventTypeEnum EventTypeEnum::THEO_PRICE{
    DXFG_EVENT_THEO_PRICE, "THEO_PRICE", "TheoPrice", true, true, true};
constinit const EventTypeEnum EventTypeEnum::TRADE{DXFG_EVENT_TRADE, "TRADE", "Trade", true};
constinit const EventTypeEnum EventTypeEnum::TRADE_ETH{DXFG_EVENT_TRADE_ETH, "TRADE_ETH", "TradeETH", true};
constinit const EventTypeEnum EventTypeEnum::CONFIGURATION{
    DXFG_EVENT_CONFIGURATION, "CONFIGURATION", "Configuration", true, false, false, false};
constinit const EventTypeEnum EventTypeEnum::MESSAGE{
    DXFG_EVENT_MESSAGE, "MESSAGE", "Message", false, false, false, false};
constinit const EventTypeEnum EventTypeEnum::TIME_AND_SALE{
    DXFG_EVENT_TIME_AND_SALE, "TIME_AND_SALE", "TimeAndSale", false, true, true};

// const EventTypeEnum EventTypeEnum::ORDER_BASE{DXFG_EVENT_ORDER_BASE, "ORDER_BASE", "OrderBase", false, true};

constinit const EventTypeEnum EventTypeEnum::ORDER{DXFG_EVENT_ORDER, "ORDER", "Order", false, true};
constinit const EventTypeEnum EventTypeEnum::ANALYTIC_ORDER{DXFG_EVENT_ANALYTIC_ORDER, "ANALYTIC_ORDER",
                                                            "AnalyticOrder", false, true};
constinit const EventTypeEnum EventTypeEnum::OTC_MARKETS_ORDER{DXFG_EVENT_OTC_MARKETS_ORDER, "OTC_MARKETS_ORDER",
                                                               "OtcMarketsOrder", false, true};
constinit const EventTypeEnum EventTypeEnum::SPREAD_ORDER{DXFG_EVENT_SPREAD_ORDER, "SPREAD_ORDER", "SpreadOrder", false,
                                                          true};
constinit const EventTypeEnum EventTypeEnum::SERIES{DXFG_EVENT_SERIES, "SERIES", "Series", false, true};
constinit const EventTypeEnum EventTypeEnum::OPTION_SALE{DXFG_EVENT_OPTION_SALE, "OPTION_SALE", "OptionSale", false,
                                                         true};
constinit const EventTypeEnum EventTypeEnum::TEXT_MESSAGE{
    DXFG_EVENT_TEXT_MESSAGE, "TEXT_MESSAGE", "TextMessage", false, false, false, false};
constinit const EventTypeEnum EventTypeEnum::ORDER_IMBALANCE{DXFG_EVENT_ORDER_IMBALANCE, "ORDER_IMBALANCE",
                                                             "OrderImbalance", false};
constinit const EventTypeEnum EventTypeEnum::NUAM_ORDER{DXFG_EVENT_NUAM_ORDER, "NUAM_ORDER", "NuamOrder", false,
                                                        true};
constinit const EventTypeEnum EventTypeEnum::NUAM_TRADE{DXFG_EVENT_NUAM_TRADE, "NUAM_TRADE", "NuamTrade", true};
constinit const EventTypeEnum EventTypeEnum::NUAM_TIME_AND_SALE{DXFG_EVENT_NUAM_TIME_AND_SALE, "NUAM_TIME_AND_SALE",
                                                                "NuamTimeAndSale", false, true, true};

namespace {

constinit const std::array ALL_EVENT_TYPES{
    std::cref(EventTypeEnum::QUOTE),
    std::cref(EventTypeEnum::PROFILE),
    std::cref(EventTypeEnum::SUMMARY),
    std::cref(EventTypeEnum::GREEKS),
    std::cref(EventTypeEnum::CANDLE),
    std::cref(EventTypeEnum::UNDERLYING),
    std::cref(EventTypeEnum::THEO_PRICE),
    std::cref(EventTypeEnum::TRADE),
    std::cref(EventTypeEnum::TRADE_ETH),
    // std::cref(EventTypeEnum::CONFIGURATION),
    std::cref(EventTypeEnum::MESSAGE),
    std::cref(EventTypeEnum::TIME_AND_SALE),
    std::cref(EventTypeEnum::ORDER),
    std::cref(EventTypeEnum::ANALYTIC_ORDER),
    std::cref(EventTypeEnum::OTC_MARKETS_ORDER),
    std::cref(EventTypeEnum::SPREAD_ORDER),
    std::cref(EventTypeEnum::SERIES),
    std::cref(EventTypeEnum::OPTION_SALE),
    std::cref(EventTypeEnum::TEXT_MESSAGE),
    std::cref(EventTypeEnum::ORDER_IMBALANCE),
    std::cref(EventTypeEnum::NUAM_ORDER),
    std::cref(EventTypeEnum::NUAM_TRADE),
    std::cref(EventTypeEnum::NUAM_TIME_AND_SALE),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL{ALL_EVENT_TYPES};

namespace {

// The keys repeat the names, the class names and the ids of the event types: a constinit constant is not read in a
// constant expression (ConstantsTest checks them).

constexpr auto ALL_EVENT_TYPES_BY_NAME = ConstantMap<std::string_view, EventTypeEnum>::sorted({
    {"QUOTE", EventTypeEnum::QUOTE},
    {"PROFILE", EventTypeEnum::PROFILE},
    {"SUMMARY", EventTypeEnum::SUMMARY},
    {"GREEKS", EventTypeEnum::GREEKS},
    {"CANDLE", EventTypeEnum::CANDLE},
    {"UNDERLYING", EventTypeEnum::UNDERLYING},
    {"THEO_PRICE", EventTypeEnum::THEO_PRICE},
    {"TRADE", EventTypeEnum::TRADE},
    {"TRADE_ETH", EventTypeEnum::TRADE_ETH},
    {"MESSAGE", EventTypeEnum::MESSAGE},
    {"TIME_AND_SALE", EventTypeEnum::TIME_AND_SALE},
    {"ORDER", EventTypeEnum::ORDER},
    {"ANALYTIC_ORDER", EventTypeEnum::ANALYTIC_ORDER},
    {"OTC_MARKETS_ORDER", EventTypeEnum::OTC_MARKETS_ORDER},
    {"SPREAD_ORDER", EventTypeEnum::SPREAD_ORDER},
    {"SERIES", EventTypeEnum::SERIES},
    {"OPTION_SALE", EventTypeEnum::OPTION_SALE},
    {"TEXT_MESSAGE", EventTypeEnum::TEXT_MESSAGE},
    {"ORDER_IMBALANCE", EventTypeEnum::ORDER_IMBALANCE},
    {"NUAM_ORDER", EventTypeEnum::NUAM_ORDER},
    {"NUAM_TRADE", EventTypeEnum::NUAM_TRADE},
    {"NUAM_TIME_AND_SALE", EventTypeEnum::NUAM_TIME_AND_SALE},
});

constexpr auto ALL_EVENT_TYPES_BY_CLASS_NAME = ConstantMap<std::string_view, EventTypeEnum>::sorted({
    {"Quote", EventTypeEnum::QUOTE},
    {"Profile", EventTypeEnum::PROFILE},
    {"Summary", EventTypeEnum::SUMMARY},
    {"Greeks", EventTypeEnum::GREEKS},
    {"Candle", EventTypeEnum::CANDLE},
    {"Underlying", EventTypeEnum::UNDERLYING},
    {"TheoPrice", EventTypeEnum::THEO_PRICE},
    {"Trade", EventTypeEnum::TRADE},
    {"TradeETH", EventTypeEnum::TRADE_ETH},
    {"Message", EventTypeEnum::MESSAGE},
    {"TimeAndSale", EventTypeEnum::TIME_AND_SALE},
    {"Order", EventTypeEnum::ORDER},
    {"AnalyticOrder", EventTypeEnum::ANALYTIC_ORDER},
    {"OtcMarketsOrder", EventTypeEnum::OTC_MARKETS_ORDER},
    {"SpreadOrder", EventTypeEnum::SPREAD_ORDER},
    {"Series", EventTypeEnum::SERIES},
    {"OptionSale", EventTypeEnum::OPTION_SALE},
    {"TextMessage", EventTypeEnum::TEXT_MESSAGE},
    {"OrderImbalance", EventTypeEnum::ORDER_IMBALANCE},
    {"NuamOrder", EventTypeEnum::NUAM_ORDER},
    {"NuamTrade", EventTypeEnum::NUAM_TRADE},
    {"NuamTimeAndSale", EventTypeEnum::NUAM_TIME_AND_SALE},
});

constexpr auto ALL_EVENT_TYPES_BY_ID = ConstantMap<std::uint32_t, EventTypeEnum>::sorted({
    {DXFG_EVENT_QUOTE, EventTypeEnum::QUOTE},
    {DXFG_EVENT_PROFILE, EventTypeEnum::PROFILE},
    {DXFG_EVENT_SUMMARY, EventTypeEnum::SUMMARY},
    {DXFG_EVENT_GREEKS, EventTypeEnum::GREEKS},
    {DXFG_EVENT_CANDLE, EventTypeEnum::CANDLE},
    {DXFG_EVENT_UNDERLYING, EventTypeEnum::UNDERLYING},
    {DXFG_EVENT_THEO_PRICE, EventTypeEnum::THEO_PRICE},
    {DXFG_EVENT_TRADE, EventTypeEnum::TRADE},
    {DXFG_EVENT_TRADE_ETH, EventTypeEnum::TRADE_ETH},
    {DXFG_EVENT_MESSAGE, EventTypeEnum::MESSAGE},
    {DXFG_EVENT_TIME_AND_SALE, EventTypeEnum::TIME_AND_SALE},
    {DXFG_EVENT_ORDER, EventTypeEnum::ORDER},
    {DXFG_EVENT_ANALYTIC_ORDER, EventTypeEnum::ANALYTIC_ORDER},
    {DXFG_EVENT_OTC_MARKETS_ORDER, EventTypeEnum::OTC_MARKETS_ORDER},
    {DXFG_EVENT_SPREAD_ORDER, EventTypeEnum::SPREAD_ORDER},
    {DXFG_EVENT_SERIES, EventTypeEnum::SERIES},
    {DXFG_EVENT_OPTION_SALE, EventTypeEnum::OPTION_SALE},
    {DXFG_EVENT_TEXT_MESSAGE, EventTypeEnum::TEXT_MESSAGE},
    {DXFG_EVENT_ORDER_IMBALANCE, EventTypeEnum::ORDER_IMBALANCE},
    {DXFG_EVENT_NUAM_ORDER, EventTypeEnum::NUAM_ORDER},
    {DXFG_EVENT_NUAM_TRADE, EventTypeEnum::NUAM_TRADE},
    {DXFG_EVENT_NUAM_TIME_AND_SALE, EventTypeEnum::NUAM_TIME_AND_SALE},
});

static_assert(ALL_EVENT_TYPES_BY_NAME.size() == std::tuple_size_v<decltype(ALL_EVENT_TYPES)>);
static_assert(ALL_EVENT_TYPES_BY_CLASS_NAME.size() == std::tuple_size_v<decltype(ALL_EVENT_TYPES)>);
static_assert(ALL_EVENT_TYPES_BY_ID.size() == std::tuple_size_v<decltype(ALL_EVENT_TYPES)>);

} // namespace

constinit const ConstantMap<std::string_view, EventTypeEnum> EventTypeEnum::ALL_BY_NAME{ALL_EVENT_TYPES_BY_NAME};
constinit const ConstantMap<std::string_view, EventTypeEnum> EventTypeEnum::ALL_BY_CLASS_NAME{
    ALL_EVENT_TYPES_BY_CLASS_NAME};
constinit const ConstantMap<std::uint32_t, EventTypeEnum> EventTypeEnum::ALL_BY_ID{ALL_EVENT_TYPES_BY_ID};

EventTypeEnum::EventTypeEnum() noexcept : EventTypeEnum{static_cast<std::uint32_t>(-1), "INVALID", "Invalid", false} {
}

std::uint32_t EventTypeEnum::getId() const noexcept {
    return id_;
}

std::string EventTypeEnum::getName() const {
    return std::string{name_};
}

std::string EventTypeEnum::getClassName() const {
    return std::string{className_};
}

bool EventTypeEnum::operator==(const EventTypeEnum &eventTypeEnum) const noexcept {
    return id_ == eventTypeEnum.id_;
}

bool EventTypeEnum::operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum) const noexcept {
    return id_ == eventTypeEnum.get().id_;
}

bool EventTypeEnum::operator<(const EventTypeEnum &eventTypeEnum) const noexcept {
    return id_ < eventTypeEnum.id_;
}

bool EventTypeEnum::operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum) const noexcept {
    return id_ < eventTypeEnum.get().id_;
}

bool EventTypeEnum::isLasting() const noexcept {
    return isLasting_;
}

bool EventTypeEnum::isIndexed() const noexcept {
    return isIndexed_;
}

bool EventTypeEnum::isTimeSeries() const noexcept {
    return isTimeSeries_;
}

bool EventTypeEnum::isOnlyIndexed() const noexcept {
    return isOnlyIndexed_;
}

bool EventTypeEnum::isMarket() const noexcept {
    return isMarket_;
}

EventTypeEnum literals::operator""_et(const char *eventTypeString, size_t) noexcept {
    const std::string_view name{eventTypeString};

    for (const auto &eventType : EventTypeEnum::ALL) {
        if (eventType.get().name_ == name) {
            return eventType;
        }
    }

    return EventTypeEnum::INVALID_EVENT_TYPE;
}

bool operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum2) noexcept {
    return eventTypeEnum1.get().id_ == eventTypeEnum2.get().id_;
}

bool operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                const EventTypeEnum &eventTypeEnum2) noexcept {
    return eventTypeEnum1.get().id_ == eventTypeEnum2.id_;
}

bool operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
               const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum2) noexcept {
    return eventTypeEnum1.get().id_ < eventTypeEnum2.get().id_;
}

bool operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
               const EventTypeEnum &eventTypeEnum2) noexcept {
    return eventTypeEnum1.get().id_ < eventTypeEnum2.id_;
}
DXFCPP_END_NAMESPACE