// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

EventTypeEnum::EventTypeEnum(std::uint32_t id, const StringLike &name, const StringLike &className, bool isLasting,
                             bool isIndexed, bool isTimeSeries, bool isMarket) noexcept
    : id_{id}, name_{std::string(name)}, className_{std::string(className)}, isLasting_{isLasting},
      isIndexed_{isIndexed || isTimeSeries}, isTimeSeries_{isTimeSeries}, isOnlyIndexed_{isIndexed && !isTimeSeries},
      isMarket_{isMarket} {
}

const EventTypeEnum EventTypeEnum::INVALID_EVENT_TYPE{};

const EventTypeEnum EventTypeEnum::QUOTE{DXFG_EVENT_QUOTE, "QUOTE", "Quote", true};
const EventTypeEnum EventTypeEnum::PROFILE{DXFG_EVENT_PROFILE, "PROFILE", "Profile", true};
const EventTypeEnum EventTypeEnum::SUMMARY{DXFG_EVENT_SUMMARY, "SUMMARY", "Summary", true};
const EventTypeEnum EventTypeEnum::GREEKS{DXFG_EVENT_GREEKS, "GREEKS", "Greeks", true, true, true};
const EventTypeEnum EventTypeEnum::CANDLE{DXFG_EVENT_CANDLE, "CANDLE", "Candle", true, true, true, false};

// const EventTypeEnum EventTypeEnum::DAILY_CANDLE{DXFG_EVENT_DAILY_CANDLE, "DAILY_CANDLE", "DailyCandle", true, true,
// true, false};

const EventTypeEnum EventTypeEnum::UNDERLYING{DXFG_EVENT_UNDERLYING, "UNDERLYING", "Underlying", true, true, true};
const EventTypeEnum EventTypeEnum::THEO_PRICE{DXFG_EVENT_THEO_PRICE, "THEO_PRICE", "TheoPrice", true, true, true};
const EventTypeEnum EventTypeEnum::TRADE{DXFG_EVENT_TRADE, "TRADE", "Trade", true};
const EventTypeEnum EventTypeEnum::TRADE_ETH{DXFG_EVENT_TRADE_ETH, "TRADE_ETH", "TradeETH", true};
const EventTypeEnum EventTypeEnum::CONFIGURATION{
    DXFG_EVENT_CONFIGURATION, "CONFIGURATION", "Configuration", true, false, false, false};
const EventTypeEnum EventTypeEnum::MESSAGE{DXFG_EVENT_MESSAGE, "MESSAGE", "Message", false, false, false, false};
const EventTypeEnum EventTypeEnum::TIME_AND_SALE{
    DXFG_EVENT_TIME_AND_SALE, "TIME_AND_SALE", "TimeAndSale", false, true, true};

// const EventTypeEnum EventTypeEnum::ORDER_BASE{DXFG_EVENT_ORDER_BASE, "ORDER_BASE", "OrderBase", false, true};

const EventTypeEnum EventTypeEnum::ORDER{DXFG_EVENT_ORDER, "ORDER", "Order", false, true};
const EventTypeEnum EventTypeEnum::ANALYTIC_ORDER{DXFG_EVENT_ANALYTIC_ORDER, "ANALYTIC_ORDER", "AnalyticOrder", false,
                                                  true};
const EventTypeEnum EventTypeEnum::OTC_MARKETS_ORDER{DXFG_EVENT_OTC_MARKETS_ORDER, "OTC_MARKETS_ORDER",
                                                     "OtcMarketsOrder", false, true};
const EventTypeEnum EventTypeEnum::SPREAD_ORDER{DXFG_EVENT_SPREAD_ORDER, "SPREAD_ORDER", "SpreadOrder", false, true};
const EventTypeEnum EventTypeEnum::SERIES{DXFG_EVENT_SERIES, "SERIES", "Series", false, true};
const EventTypeEnum EventTypeEnum::OPTION_SALE{DXFG_EVENT_OPTION_SALE, "OPTION_SALE", "OptionSale", false, true};
const EventTypeEnum EventTypeEnum::TEXT_MESSAGE{
    DXFG_EVENT_TEXT_MESSAGE, "TEXT_MESSAGE", "TextMessage", false, false, false, false};

const std::vector<std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL{
    std::cref(QUOTE),
    std::cref(PROFILE),
    std::cref(SUMMARY),
    std::cref(GREEKS),
    std::cref(CANDLE),
    std::cref(UNDERLYING),
    std::cref(THEO_PRICE),
    std::cref(TRADE),
    std::cref(TRADE_ETH),
    // std::cref(CONFIGURATION),
    std::cref(MESSAGE),
    std::cref(TIME_AND_SALE),
    std::cref(ORDER),
    std::cref(ANALYTIC_ORDER),
    std::cref(OTC_MARKETS_ORDER),
    std::cref(SPREAD_ORDER),
    std::cref(SERIES),
    std::cref(OPTION_SALE),
    std::cref(TEXT_MESSAGE),
};

const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL_BY_NAME =
    [](auto types) {
        std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> result{};

        for (auto &&t : types) {
            result.emplace(t.get().getName(), t);
        }

        return result;
    }(ALL);

const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL_BY_CLASS_NAME =
    [](auto types) {
        std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> result{};

        for (auto &&t : types) {
            result.emplace(t.get().getClassName(), t);
        }

        return result;
    }(ALL);

const std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL_BY_ID =
    [](auto types) {
        std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>> result{};

        for (auto &&t : types) {
            result.emplace(t.get().getId(), t);
        }

        return result;
    }(ALL);

EventTypeEnum::EventTypeEnum() noexcept : EventTypeEnum{static_cast<std::uint32_t>(-1), "INVALID", "Invalid", false} {
}

EventTypeEnum::~EventTypeEnum() noexcept {
}

std::uint32_t EventTypeEnum::getId() const noexcept {
    return id_;
}

const std::string &EventTypeEnum::getName() const & noexcept {
    return name_;
}

const std::string &EventTypeEnum::getClassName() const & noexcept {
    return className_;
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
    return isOnlyIndexed_;
}

EventTypeEnum literals::operator""_et(const char *eventTypeString, size_t) noexcept {
    if (auto it = EventTypeEnum::ALL_BY_NAME.find(eventTypeString); it != EventTypeEnum::ALL_BY_NAME.end()) {
        return it->second;
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