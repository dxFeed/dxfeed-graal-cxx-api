// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "EventFlag.hpp"
#include "EventType.hpp"
#include "EventTypeEnum.hpp"
#include "IndexedEvent.hpp"
#include "IndexedEventSource.hpp"
#include "LastingEvent.hpp"
#include "TimeSeriesEvent.hpp"
#include "entity/Entity.hpp"
#include "entity/SharedEntity.hpp"
#include "event/market/MarketEvent.hpp"
#include "event/market/Quote.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Profile information snapshot that contains security instrument description.
 * It represents the most recent information that is available about the traded security
 * on the market at any given moment of time.
 *
 */
class Profile final : public MarketEvent, public LastingEvent {
    friend struct EventMapper;

    /*
     * Flags property has several significant bits that are packed into an integer in the following way:
     *   31..4     3    2    1    0
     * +--------+----+----+----+----+
     * |        |   SSR   |  Status |
     * +--------+----+----+----+----+
     */

    // SSR values are taken from ShortSaleRestriction enum.
    static const std::uint32_t SSR_MASK = 3;
    static const std::uint32_t SSR_SHIFT = 2;

    // STATUS values are taken from TradingStatus enum.
    static const std::uint32_t STATUS_MASK = 3;
    static const std::uint32_t STATUS_SHIFT = 0;

    struct Data {
        std::string description{};
        std::string statusReason{};
        std::int64_t haltStartTime{};
        std::int64_t haltEndTime{};
        double highLimitPrice = detail::math::NaN;
        double lowLimitPrice = detail::math::NaN;
        double high52WeekPrice = detail::math::NaN;
        double low52WeekPrice = detail::math::NaN;
        double beta = detail::math::NaN;
        double earningsPerShare = detail::math::NaN;
        double dividendFrequency = detail::math::NaN;
        double exDividendAmount = detail::math::NaN;
        std::int32_t exDividendDayId{};
        double shares = detail::math::NaN;
        double freeFloat = detail::math::NaN;
        std::int32_t flags{};
    } data_;

  public:
    /// Creates new profile with default values.
    Profile() noexcept : data_{} {}

    /**
     * Creates new profile with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Profile(std::string eventSymbol) noexcept : MarketEvent(std::move(eventSymbol)), data_{} {}

    /**
     * Returns description of the security instrument.
     *
     * @return description of the security instrument.
     */
    const std::string &getDescription() const & { return data_.description; }

    /**
     * Changes description of the security instrument.
     *
     * @param description description of the security instrument.
     */
    void setDescription(std::string description) { data_.description = std::move(description); }
};

struct EventMapper {
    static std::vector<std::shared_ptr<EventType>> fromGraalNativeList(void *graalNativeList);
};

} // namespace dxfcpp