// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Profile.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Profile::TYPE = EventTypeEnum::PROFILE;

void Profile::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    data_ = {
        .description = toStringOpt(graalProfile->description),
        .statusReason = toStringOpt(graalProfile->status_reason),
        .haltStartTime = graalProfile->halt_start_time,
        .haltEndTime = graalProfile->halt_end_time,
        .highLimitPrice = graalProfile->high_limit_price,
        .lowLimitPrice = graalProfile->low_limit_price,
        .high52WeekPrice = graalProfile->high_52_week_price,
        .low52WeekPrice = graalProfile->low_52_week_price,
        .beta = graalProfile->beta,
        .earningsPerShare = graalProfile->earnings_per_share,
        .dividendFrequency = graalProfile->dividend_frequency,
        .exDividendAmount = graalProfile->ex_dividend_amount,
        .exDividendDayId = graalProfile->ex_dividend_day_id,
        .shares = graalProfile->shares,
        .freeFloat = graalProfile->free_float,
        .flags = graalProfile->flags,
    };
}

void Profile::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    graalProfile->market_event.event_type.clazz = DXFG_EVENT_PROFILE;
    graalProfile->description = createCString(data_.description);
    graalProfile->status_reason = createCString(data_.statusReason);
    graalProfile->halt_start_time = data_.haltStartTime;
    graalProfile->halt_end_time = data_.haltEndTime;
    graalProfile->high_limit_price = data_.highLimitPrice;
    graalProfile->low_limit_price = data_.lowLimitPrice;
    graalProfile->high_52_week_price = data_.high52WeekPrice;
    graalProfile->low_52_week_price = data_.low52WeekPrice;
    graalProfile->beta = data_.beta;
    graalProfile->earnings_per_share = data_.earningsPerShare;
    graalProfile->dividend_frequency = data_.dividendFrequency;
    graalProfile->ex_dividend_amount = data_.exDividendAmount;
    graalProfile->ex_dividend_day_id = data_.exDividendDayId;
    graalProfile->shares = data_.shares;
    graalProfile->free_float = data_.freeFloat;
    graalProfile->flags = data_.flags;
}

void Profile::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    const auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    delete[] graalProfile->description;
    delete[] graalProfile->status_reason;
}

std::shared_ptr<Profile> Profile::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Profile. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_PROFILE) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Profile. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_PROFILE)));
    }

    auto profile = std::make_shared<Profile>();

    profile->fillData(graalNative);

    return profile;
}

void *Profile::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalProfile = new dxfg_profile_t{};

    fillGraalData(graalProfile);

    return graalProfile;
}

void Profile::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_PROFILE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Profile's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_PROFILE)));
    }

    const auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalProfile;
}

void Profile::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Profile>(); other) {
        data_ = other->data_;
    }
}

Profile::Profile() noexcept {
}

Profile::Profile(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const std::string &Profile::getDescription() const & noexcept {
    if (!data_.description) {
        return String::NUL;
    }

    return data_.description.value();
}

const std::optional<std::string> &Profile::getDescriptionOpt() const & noexcept {
    return data_.description;
}

void Profile::setDescription(const StringLike &description) noexcept {
    data_.description = std::string(description);
}

const ShortSaleRestriction &Profile::getShortSaleRestriction() const & noexcept {
    return ShortSaleRestriction::valueOf(getBits(data_.flags, SSR_MASK, SSR_SHIFT));
}

void Profile::setShortSaleRestriction(const ShortSaleRestriction &restriction) noexcept {
    data_.flags = setBits(data_.flags, SSR_MASK, SSR_SHIFT, restriction.getCode());
}

bool Profile::isShortSaleRestricted() const noexcept {
    return getShortSaleRestriction() == ShortSaleRestriction::ACTIVE;
}

const TradingStatus &Profile::getTradingStatus() const & noexcept {
    return TradingStatus::valueOf(getBits(data_.flags, STATUS_MASK, STATUS_SHIFT));
}

void Profile::setTradingStatus(const TradingStatus &status) noexcept {
    data_.flags = setBits(data_.flags, STATUS_MASK, STATUS_SHIFT, status.getCode());
}

bool Profile::isTradingHalted() const noexcept {
    return getTradingStatus() == TradingStatus::HALTED;
}

const std::string &Profile::getStatusReason() const & noexcept {
    if (!data_.statusReason) {
        return String::NUL;
    }

    return data_.statusReason.value();
}

const std::optional<std::string> &Profile::getStatusReasonOpt() const & noexcept {
    return data_.statusReason;
}

void Profile::setStatusReason(const StringLike &statusReason) noexcept {
    data_.statusReason = std::string(statusReason);
}

std::int64_t Profile::getHaltStartTime() const noexcept {
    return data_.haltStartTime;
}

void Profile::setHaltStartTime(std::int64_t haltStartTime) noexcept {
    data_.haltStartTime = haltStartTime;
}

std::int64_t Profile::getHaltEndTime() const noexcept {
    return data_.haltEndTime;
}

void Profile::setHaltEndTime(std::int64_t haltEndTime) noexcept {
    data_.haltEndTime = haltEndTime;
}

double Profile::getHighLimitPrice() const noexcept {
    return data_.highLimitPrice;
}

void Profile::setHighLimitPrice(double highLimitPrice) noexcept {
    data_.highLimitPrice = highLimitPrice;
}

double Profile::getLowLimitPrice() const noexcept {
    return data_.lowLimitPrice;
}

void Profile::setLowLimitPrice(double lowLimitPrice) noexcept {
    data_.lowLimitPrice = lowLimitPrice;
}

double Profile::getHigh52WeekPrice() const noexcept {
    return data_.high52WeekPrice;
}

void Profile::setHigh52WeekPrice(double high52WeekPrice) noexcept {
    data_.high52WeekPrice = high52WeekPrice;
}

double Profile::getLow52WeekPrice() const noexcept {
    return data_.low52WeekPrice;
}

void Profile::setLow52WeekPrice(double low52WeekPrice) noexcept {
    data_.low52WeekPrice = low52WeekPrice;
}

double Profile::getBeta() const noexcept {
    return data_.beta;
}

void Profile::setBeta(double beta) noexcept {
    data_.beta = beta;
}

double Profile::getEarningsPerShare() const noexcept {
    return data_.earningsPerShare;
}

void Profile::setEarningsPerShare(double earningsPerShare) noexcept {
    data_.earningsPerShare = earningsPerShare;
}

double Profile::getDividendFrequency() const noexcept {
    return data_.dividendFrequency;
}

void Profile::setDividendFrequency(double dividendFrequency) noexcept {
    data_.dividendFrequency = dividendFrequency;
}

double Profile::getExDividendAmount() const noexcept {
    return data_.exDividendAmount;
}

void Profile::setExDividendAmount(double exDividendAmount) noexcept {
    data_.exDividendAmount = exDividendAmount;
}

std::int32_t Profile::getExDividendDayId() const noexcept {
    return data_.exDividendDayId;
}

void Profile::setExDividendDayId(std::int32_t exDividendDayId) noexcept {
    data_.exDividendDayId = exDividendDayId;
}

double Profile::getShares() const noexcept {
    return data_.shares;
}

void Profile::setShares(double shares) noexcept {
    data_.shares = shares;
}

double Profile::getFreeFloat() const noexcept {
    return data_.freeFloat;
}

void Profile::setFreeFloat(double freeFloat) noexcept {
    data_.freeFloat = freeFloat;
}

std::string Profile::toString() const {
    return fmt::format(
        "Profile{{{}, eventTime={}, description='{}', SSR={}, status={}, statusReason='{}', "
        "haltStartTime={}, haltEndTime={}, highLimitPrice={}, lowLimitPrice={}, high52WeekPrice={}, "
        "low52WeekPrice={}, beta={}, earningsPerShare={}, dividendFrequency={}, "
        "exDividendAmount={}, exDividendDay={}, shares={}, freeFloat={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getDescription(), getShortSaleRestriction().toString(), getTradingStatus().toString(), getStatusReason(),
        TimeFormat::DEFAULT.format(getHaltStartTime()), TimeFormat::DEFAULT.format(getHaltEndTime()),
        dxfcpp::toString(getHighLimitPrice()), dxfcpp::toString(getLowLimitPrice()),
        dxfcpp::toString(getHigh52WeekPrice()), dxfcpp::toString(getLow52WeekPrice()), dxfcpp::toString(getBeta()),
        dxfcpp::toString(getEarningsPerShare()), dxfcpp::toString(getDividendFrequency()),
        dxfcpp::toString(getExDividendAmount()), day_util::getYearMonthDayByDayId(getExDividendDayId()),
        dxfcpp::toString(getShares()), dxfcpp::toString(getFreeFloat()));
}

DXFCPP_END_NAMESPACE