// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Summary.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Summary::TYPE = EventTypeEnum::SUMMARY;

void Summary::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

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

    const auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

    graalSummary->market_event.event_type.clazz = DXFG_EVENT_SUMMARY;
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

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SUMMARY) {
        throw InvalidArgumentException(fmt::format("Unable to create Summary. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_SUMMARY)));
    }

    auto summary = std::make_shared<Summary>();

    summary->fillData(graalNative);

    return summary;
}

void *Summary::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSummary = new dxfg_summary_t{};

    fillGraalData(graalSummary);

    return graalSummary;
}

void Summary::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SUMMARY) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Summary's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_SUMMARY)));
    }

    const auto graalSummary = static_cast<dxfg_summary_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalSummary;
}

void Summary::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Summary>(); other) {
        data_ = other->data_;
    }
}

Summary::Summary() noexcept {
}

Summary::Summary(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

std::int32_t Summary::getDayId() const noexcept {
    return data_.dayId;
}

void Summary::setDayId(std::int32_t dayId) noexcept {
    data_.dayId = dayId;
}

double Summary::getDayOpenPrice() const noexcept {
    return data_.dayOpenPrice;
}

void Summary::setDayOpenPrice(double dayOpenPrice) noexcept {
    data_.dayOpenPrice = dayOpenPrice;
}

double Summary::getDayHighPrice() const noexcept {
    return data_.dayHighPrice;
}

void Summary::setDayHighPrice(double dayHighPrice) noexcept {
    data_.dayHighPrice = dayHighPrice;
}

double Summary::getDayLowPrice() const noexcept {
    return data_.dayLowPrice;
}

void Summary::setDayLowPrice(double dayLowPrice) noexcept {
    data_.dayLowPrice = dayLowPrice;
}

double Summary::getDayClosePrice() const noexcept {
    return data_.dayClosePrice;
}

void Summary::setDayClosePrice(double dayClosePrice) noexcept {
    data_.dayClosePrice = dayClosePrice;
}

const PriceType &Summary::getDayClosePriceType() const & noexcept {
    return PriceType::valueOf(getBits(data_.flags, DAY_CLOSE_PRICE_TYPE_MASK, DAY_CLOSE_PRICE_TYPE_SHIFT));
}

void Summary::setDayClosePriceType(const PriceType &type) noexcept {
    data_.flags = setBits(data_.flags, DAY_CLOSE_PRICE_TYPE_MASK, DAY_CLOSE_PRICE_TYPE_SHIFT, type.getCode());
}

std::int32_t Summary::getPrevDayId() const noexcept {
    return data_.prevDayId;
}

void Summary::setPrevDayId(std::int32_t prevDayId) noexcept {
    data_.prevDayId = prevDayId;
}

double Summary::getPrevDayClosePrice() const noexcept {
    return data_.prevDayClosePrice;
}

void Summary::setPrevDayClosePrice(double prevDayClosePrice) noexcept {
    data_.prevDayClosePrice = prevDayClosePrice;
}

const PriceType &Summary::getPrevDayClosePriceType() const & noexcept {
    return PriceType::valueOf(getBits(data_.flags, PREV_DAY_CLOSE_PRICE_TYPE_MASK, PREV_DAY_CLOSE_PRICE_TYPE_SHIFT));
}

void Summary::setPrevDayClosePriceType(const PriceType &type) noexcept {
    data_.flags = setBits(data_.flags, PREV_DAY_CLOSE_PRICE_TYPE_MASK, PREV_DAY_CLOSE_PRICE_TYPE_SHIFT, type.getCode());
}

double Summary::getPrevDayVolume() const noexcept {
    return data_.prevDayVolume;
}

void Summary::setPrevDayVolume(double prevDayVolume) noexcept {
    data_.prevDayVolume = prevDayVolume;
}

std::int64_t Summary::getOpenInterest() const noexcept {
    return data_.openInterest;
}

void Summary::setOpenInterest(std::int64_t openInterest) noexcept {
    data_.openInterest = openInterest;
}

std::string Summary::toString() const {
    return fmt::format("Summary{{{}, eventTime={}, day={}, dayOpen={}, dayHigh={}, dayLow='{}', "
                       "dayClose={}, dayCloseType={}, prevDay={}, prevDayClose={}, prevDayCloseType={}, "
                       "prevDayVolume={}, openInterest={}}}",
                       MarketEvent::getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
                       day_util::getYearMonthDayByDayId(getDayId()), dxfcpp::toString(getDayOpenPrice()),
                       dxfcpp::toString(getDayHighPrice()), dxfcpp::toString(getDayLowPrice()),
                       dxfcpp::toString(getDayClosePrice()), getDayClosePriceType().toString(),
                       day_util::getYearMonthDayByDayId(getPrevDayId()), dxfcpp::toString(getPrevDayClosePrice()),
                       getPrevDayClosePriceType().toString(), dxfcpp::toString(getPrevDayVolume()), getOpenInterest());
}

DXFCPP_END_NAMESPACE