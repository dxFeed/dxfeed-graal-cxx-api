// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Summary::TYPE = EventTypeEnum::SUMMARY;

void Summary::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

    data_ = {
        .dayId = graalSummary->day_id,
        .dayOpenPrice = graalSummary->day_open_price,
        .dayHighPrice = graalSummary->day_high_price,
        .dayLowPrice = graalSummary->day_low_price,
        .dayClosePrice = graalSummary->day_close_price,
        .prevDayId = graalSummary->prev_day_id,
        .prevDayClosePrice = graalSummary->prev_day_close_price,
        .prevDayVolume = graalSummary->prev_day_volume,
        .openInterest = graalSummary->open_interest,
        .flags = graalSummary->flags,
    };
}

void Summary::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

    graalSummary->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_SUMMARY;
    graalSummary->day_id = data_.dayId;
    graalSummary->day_open_price = data_.dayOpenPrice;
    graalSummary->day_high_price = data_.dayHighPrice;
    graalSummary->day_low_price = data_.dayLowPrice;
    graalSummary->day_close_price = data_.dayClosePrice;
    graalSummary->prev_day_id = data_.prevDayId;
    graalSummary->prev_day_close_price = data_.prevDayClosePrice;
    graalSummary->prev_day_volume = data_.prevDayVolume;
    graalSummary->open_interest = data_.openInterest;
    graalSummary->flags = data_.flags;
}

std::shared_ptr<Summary> Summary::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Summary. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_SUMMARY) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Summary. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_SUMMARY))));
    }

    auto summary = std::make_shared<Summary>();

    summary->fillData(graalNative);

    return summary;
}

std::string Summary::toString() const {
    return fmt::format(
        "Summary{{{}, eventTime={}, day={}, dayOpen={}, dayHigh={}, dayLow='{}', "
        "dayClose={}, dayCloseType={}, prevDay={}, prevDayClose={}, prevDayCloseType={}, "
        "prevDayVolume={}, openInterest={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        day_util::getYearMonthDayByDayId(getDayId()), dxfcpp::toString(getDayOpenPrice()),
        dxfcpp::toString(getDayHighPrice()), dxfcpp::toString(getDayLowPrice()), dxfcpp::toString(getDayLowPrice()),
        dxfcpp::toString(getDayClosePrice()), getDayClosePriceType().toString(),
        day_util::getYearMonthDayByDayId(getPrevDayId()), dxfcpp::toString(getPrevDayClosePrice()),
        getPrevDayClosePriceType().toString(), dxfcpp::toString(getPrevDayVolume()), getOpenInterest());
}

void *Summary::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSummary = new dxfg_summary_t{};

    fillGraalData(static_cast<void *>(graalSummary));

    return static_cast<void *>(graalSummary);
}

void Summary::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_SUMMARY) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Summary's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_SUMMARY))));
    }

    auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalSummary;
}

void Summary::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Summary>(); other) {
        data_ = other->data_;
    }
}

DXFCPP_END_NAMESPACE