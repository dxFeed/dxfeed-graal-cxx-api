// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_set>

/**
 * \addtogroup dxfcpp_event
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The enumeration type that provides additional information about the dxFeed Graal C++-API event type.
 * Useful when used as a subscription parameter.
 */
class DXFCPP_EXPORT EventTypeEnum {
    /// The dxFeed Graal Native C-API event class id corresponding to the current enum element.
    const std::uint32_t id_;

    const std::string name_;

    const std::string className_;

    // A flag that indicates that the current enum element is characterizing the Lasting (TICKER) event.
    const bool isLasting_;

    // A flag that indicates that the current enum element is characterizing the Indexed event.
    const bool isIndexed_;

    // A flag that indicates that the current enum element is characterizing the TimeSeries (HISTORY) event.
    const bool isTimeSeries_;

    // A flag that indicates that the current enum element is characterizing the OnlyIndexed (Indexed, but not
    // TimeSeries) event.
    const bool isOnlyIndexed_;

    // A flag that indicates that the current enum element is characterizing the Market event.
    const bool isMarket_;

    EventTypeEnum(std::uint32_t id, const StringLike & name, const StringLike & className, bool isLasting, bool isIndexed = false,
                  bool isTimeSeries = false, bool isMarket = true) noexcept;

    public:
    using RefSetType =
        std::unordered_set<std::reference_wrapper<const EventTypeEnum>, decltype([](auto &&eventTypeRef) {
                               return static_cast<std::size_t>(eventTypeRef.get().getId());
                           })>;

    static const EventTypeEnum INVALID_EVENT_TYPE;

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
    static const EventTypeEnum OTC_MARKETS_ORDER;
    static const EventTypeEnum SPREAD_ORDER;
    static const EventTypeEnum SERIES;
    static const EventTypeEnum OPTION_SALE;

    static const EventTypeEnum TEXT_MESSAGE;

    static const std::vector<std::reference_wrapper<const EventTypeEnum>> ALL;

    static const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> ALL_BY_NAME;

    static const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> ALL_BY_CLASS_NAME;

    static const std::unordered_map<std::uint32_t, std::reference_wrapper<const EventTypeEnum>> ALL_BY_ID;

    EventTypeEnum() noexcept;

    virtual ~EventTypeEnum() noexcept;

    /**
     * @return The dxFeed Graal Native C-API event class id
     */
    [[nodiscard]] std::uint32_t getId() const noexcept;

    /**
     * @return The current enum element name
     */
    [[nodiscard]] const std::string &getName() const & noexcept;

    /**
     * @return The current enum element class name
     */
    [[nodiscard]] const std::string &getClassName() const & noexcept;

    bool operator==(const EventTypeEnum &eventTypeEnum) const noexcept;

    bool operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum) const noexcept;

    friend bool operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                           const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum2) noexcept;

    friend bool operator==(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                           const EventTypeEnum &eventTypeEnum2) noexcept;

    bool operator<(const EventTypeEnum &eventTypeEnum) const noexcept;

    bool operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum) const noexcept;

    friend bool operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                          const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum2) noexcept;

    friend bool operator<(const std::reference_wrapper<const EventTypeEnum> &eventTypeEnum1,
                          const EventTypeEnum &eventTypeEnum2) noexcept;

    /**
     * @return `true` if the current enum element is characterizing the Lasting (TICKER) event
     */
    [[nodiscard]] bool isLasting() const noexcept;

    /**
     * @return `true` if the current enum element is characterizing the Indexed event.
     */
    [[nodiscard]] bool isIndexed() const noexcept;

    /**
     * @return `true` if the current enum element is characterizing the TimeSeries (HISTORY) event.
     */
    [[nodiscard]] bool isTimeSeries() const noexcept;

    /**
     * @return `true` if the current enum element is characterizing the OnlyIndexed (Indexed, but not TimeSeries) event.
     */
    [[nodiscard]] bool isOnlyIndexed() const noexcept;

    /**
     * @return `true` if the current enum element is characterizing the Market event.
     */
    [[nodiscard]] bool isMarket() const noexcept;
};

inline namespace literals {

/**
 * String literal that helps to construct EventTypeEnum from a char array.
 *
 * @param eventTypeString The event type name's char array
 * @return EventTypeEnum built on the char array
 */
EventTypeEnum operator""_et(const char *eventTypeString, size_t) noexcept;

} // namespace literals

DXFCPP_END_NAMESPACE

/// @}

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::EventTypeEnum> {
    std::size_t operator()(const dxfcpp::EventTypeEnum &eventType) const noexcept {
        return eventType.getId();
    }
};

template <> struct DXFCPP_EXPORT std::hash<std::reference_wrapper<const dxfcpp::EventTypeEnum>> {
    std::size_t operator()(const std::reference_wrapper<const dxfcpp::EventTypeEnum> &eventType) const noexcept {
        return eventType.get().getId();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
