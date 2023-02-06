// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_events.h>
#include <utility>

namespace dxfcpp {

/**
 * The enumeration type that provides additional information about the dxFeed C++-API event type.
 * Useful when used as a subscription parameter.
 */
class EventTypeEnum {
    /// The dxFeed Graal Native C-API event class id corresponding to the current enum element.
    dxfg_event_clazz_t dxFeedGraalNativeApiEventClazz_;

    // A flag that indicates that the current enum element is characterizing the Lasting (TICKER) event.
    bool isLasting_;

    // A flag that indicates that the current enum element is characterizing the Indexed event.
    bool isIndexed_;

    // A flag that indicates that the current enum element is characterizing the TimeSeries (HISTORY) event.
    bool isTimeSeries_;

    // A flag that indicates that the current enum element is characterizing the OnlyIndexed (Indexed, but not
    // TimeSeries) event.
    bool isOnlyIndexed_;

    EventTypeEnum(dxfg_event_clazz_t dxFeedGraalNativeApiEventClazz, bool isLasting, bool isIndexed = false,
                  bool isTimeSeries = false)
        : dxFeedGraalNativeApiEventClazz_{dxFeedGraalNativeApiEventClazz}, isLasting_{isLasting},
          isIndexed_{isIndexed || isTimeSeries}, isTimeSeries_{isTimeSeries}, isOnlyIndexed_{isIndexed &&
                                                                                             !isTimeSeries} {}

  public:
    static const EventTypeEnum QUOTE;
    static const EventTypeEnum PROFILE;
    static const EventTypeEnum SUMMARY;
    static const EventTypeEnum GREEKS;
    static const EventTypeEnum CANDLE;

    // static const EventTypeEnum DAILY_CANDLE; // deprecated

    static const EventTypeEnum UNDERLYING;
    static const EventTypeEnum THEO_PRICE;
    static const EventTypeEnum TRADE;
    static const EventTypeEnum TRADE_ETH;
    static const EventTypeEnum CONFIGURATION;
    static const EventTypeEnum MESSAGE;
    static const EventTypeEnum TIME_AND_SALE;

    // static const EventTypeEnum ORDER_BASE; // non abstract, unusable

    static const EventTypeEnum ORDER;
    static const EventTypeEnum ANALYTIC_ORDER;
    static const EventTypeEnum SPREAD_ORDER;
    static const EventTypeEnum SERIES;

    explicit EventTypeEnum() : EventTypeEnum{static_cast<dxfg_event_clazz_t>(-1), false} {}

    /**
     * @return The dxFeed Graal Native C-API event class id
     */
    unsigned getDxFeedGraalNativeApiEventClazz() const { return dxFeedGraalNativeApiEventClazz_; }

    bool operator==(const EventTypeEnum &eventTypeEnum) const {
        return dxFeedGraalNativeApiEventClazz_ == eventTypeEnum.dxFeedGraalNativeApiEventClazz_;
    }

    /**
     * @return `true` if the current enum element is characterizing the Lasting (TICKER) event
     */
    bool isLasting() const { return isLasting_; }

    /**
     * @return `true` if the current enum element is characterizing the Indexed event.
     */
    bool isIndexed() const { return isIndexed_; }

    /**
     * @return `true` if the current enum element is characterizing the TimeSeries (HISTORY) event.
     */
    bool isTimeSeries() const { return isTimeSeries_; }

    /**
     * @return `true` if the current enum element is characterizing the OnlyIndexed (Indexed, but not TimeSeries) event.
     */
    bool isOnlyIndexed() const { return isOnlyIndexed_; }
};

} // namespace dxfcpp

namespace std {

template <> struct hash<dxfcpp::EventTypeEnum> {
    std::size_t operator()(const dxfcpp::EventTypeEnum &eventType) const noexcept {
        return eventType.getDxFeedGraalNativeApiEventClazz();
    }
};

} // namespace std