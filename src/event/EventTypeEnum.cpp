// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"

#include <dxfg_api.h>

#include <array>
#include <functional>
#include <span>
#include <string>
#include <string_view>

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
};

} // namespace

constinit const std::span<const std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL{ALL_EVENT_TYPES};

EventTypeEnum::EventTypeEnum() noexcept : EventTypeEnum{static_cast<std::uint32_t>(-1), "INVALID", "Invalid", false} {
}

EventTypeEnum::~EventTypeEnum() noexcept {
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
const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> &EventTypeEnum::ALL_BY_NAME() {
    static const auto *const value =
        new std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>>([] {
            std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> result{};

            for (const auto &eventType : ALL) {
                result.emplace(eventType.get().getName(), eventType);
            }

            return result;
        }());

    return *value;
}

const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> &EventTypeEnum::ALL_BY_CLASS_NAME() {
    static const auto *const value =
        new std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>>([] {
            std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> result{};

            for (const auto &eventType : ALL) {
                result.emplace(eventType.get().getClassName(), eventType);
            }

            return result;
        }());

    return *value;
}

const std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>> &EventTypeEnum::ALL_BY_ID() {
    static const auto *const value =
        new std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>>([] {
            std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>> result{};

            for (const auto &eventType : ALL) {
                result.emplace(eventType.get().getId(), eventType);
            }

            return result;
        }());

    return *value;
}

DXFCPP_END_NAMESPACE