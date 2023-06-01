// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Profile::TYPE = EventTypeEnum::PROFILE;

std::shared_ptr<Profile> Profile::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_PROFILE) {
        return {};
    }

    try {
        auto graalProfile = bit_cast<dxfg_profile_t *>(graalNative);
        auto profile = std::make_shared<Profile>(dxfcpp::toString(graalProfile->market_event.event_symbol));

        profile->setEventTime(graalProfile->market_event.event_time);
        profile->data_ = {
            dxfcpp::toString(graalProfile->description),
            dxfcpp::toString(graalProfile->status_reason),
            graalProfile->halt_start_time,
            graalProfile->halt_end_time,
            graalProfile->high_limit_price,
            graalProfile->low_limit_price,
            graalProfile->high_52_week_price,
            graalProfile->low_52_week_price,
            graalProfile->beta,
            graalProfile->earnings_per_share,
            graalProfile->dividend_frequency,
            graalProfile->ex_dividend_amount,
            graalProfile->ex_dividend_day_id,
            graalProfile->shares,
            graalProfile->free_float,
            graalProfile->flags,
        };

        return profile;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Profile::toString() const noexcept {
    return fmt::format("Profile{{{}, eventTime={}, description='{}', SSR={}, status={}, statusReason='{}', "
                       "haltStartTime={}, haltEndTime={}, highLimitPrice={}, lowLimitPrice={}, high52WeekPrice={}, "
                       "low52WeekPrice={}, beta={}, earningsPerShare={}, dividendFrequency={}, "
                       "exDividendAmount={}, exDividendDay={}, shares={}, freeFloat={}}}",
                       MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
                       getDescription(), getShortSaleRestriction().toString(), getTradingStatus().toString(),
                       getStatusReason(), formatTimeStamp(getHaltStartTime()), formatTimeStamp(getHaltEndTime()),
                       getHighLimitPrice(), getLowLimitPrice(), getHigh52WeekPrice(), getLow52WeekPrice(),
                       getBeta(), getEarningsPerShare(), getDividendFrequency(), getExDividendAmount(),
                       day_util::getYearMonthDayByDayId(getExDividendDayId()), getShares(), getFreeFloat());
}

} // namespace dxfcpp