// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

void TimeAndSale::setExchangeCode(char exchangeCode) noexcept {
    data_.exchangeCode = utf8to16(exchangeCode);
}

const EventTypeEnum &TimeAndSale::TYPE = EventTypeEnum::TIME_AND_SALE;

void TimeAndSale::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    data_ = {
        .eventFlags = graalTimeAndSale->event_flags,
        .index = graalTimeAndSale->index,
        .timeNanoPart = graalTimeAndSale->time_nano_part,
        .exchangeCode = graalTimeAndSale->exchange_code,
        .price = graalTimeAndSale->price,
        .size = graalTimeAndSale->size,
        .bidPrice = graalTimeAndSale->bid_price,
        .askPrice = graalTimeAndSale->ask_price,
        .exchangeSaleConditions = toStringOpt(graalTimeAndSale->exchange_sale_conditions),
        .flags = graalTimeAndSale->flags,
        .buyer = toStringOpt(graalTimeAndSale->buyer),
        .seller = toStringOpt(graalTimeAndSale->seller),
    };
}

void TimeAndSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    graalTimeAndSale->market_event.event_type.clazz = DXFG_EVENT_TIME_AND_SALE;
    graalTimeAndSale->event_flags = data_.eventFlags;
    graalTimeAndSale->index = data_.index;
    graalTimeAndSale->time_nano_part = data_.timeNanoPart;
    graalTimeAndSale->exchange_code = data_.exchangeCode;
    graalTimeAndSale->price = data_.price;
    graalTimeAndSale->size = data_.size;
    graalTimeAndSale->bid_price = data_.bidPrice;
    graalTimeAndSale->ask_price = data_.askPrice;
    graalTimeAndSale->exchange_sale_conditions = createCString(data_.exchangeSaleConditions);
    graalTimeAndSale->flags = data_.flags;
    graalTimeAndSale->buyer = createCString(data_.buyer);
    graalTimeAndSale->seller = createCString(data_.seller);
}

void TimeAndSale::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    delete[] graalTimeAndSale->exchange_sale_conditions;
    delete[] graalTimeAndSale->buyer;
    delete[] graalTimeAndSale->seller;
}

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TimeAndSale. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TIME_AND_SALE) {
        throw InvalidArgumentException(fmt::format("Unable to create TimeAndSale. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_TIME_AND_SALE)));
    }

    auto timeAndSale = std::make_shared<TimeAndSale>();

    timeAndSale->fillData(graalNative);

    return timeAndSale;
}

void *TimeAndSale::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTimeAndSale = new dxfg_time_and_sale_t{};

    fillGraalData(graalTimeAndSale);

    return graalTimeAndSale;
}

void TimeAndSale::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TIME_AND_SALE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free TimeAndSale's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_TIME_AND_SALE)));
    }

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTimeAndSale;
}

void TimeAndSale::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<TimeAndSale>(); other) {
        data_ = other->data_;
    }
}

TimeAndSale::TimeAndSale() noexcept {
}

TimeAndSale::TimeAndSale(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const IndexedEventSource &TimeAndSale::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t TimeAndSale::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask TimeAndSale::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void TimeAndSale::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

void TimeAndSale::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

std::int64_t TimeAndSale::getIndex() const noexcept {
    return data_.index;
}

void TimeAndSale::setIndex(std::int64_t index) {
    data_.index = index;
}

std::int64_t TimeAndSale::getTime() const noexcept {
    return sar(data_.index, SECONDS_SHIFT) * 1000 + andOp(sar(data_.index, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void TimeAndSale::setTime(std::int64_t time) noexcept {
    data_.index = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                            sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                       getSequence());
}

std::int64_t TimeAndSale::getTimeNanos() const noexcept {
    return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
}

void TimeAndSale::setTimeNanos(std::int64_t timeNanos) noexcept {
    setTime(time_nanos_util::getMillisFromNanos(timeNanos));
    data_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
}

void TimeAndSale::setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    data_.timeNanoPart = timeNanoPart;
}

std::int32_t TimeAndSale::getTimeNanoPart() const noexcept {
    return data_.timeNanoPart;
}

std::int32_t TimeAndSale::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.index, MAX_SEQUENCE));
}

void TimeAndSale::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    data_.index = orOp(andOp(data_.index, ~MAX_SEQUENCE), sequence);
}

std::int16_t TimeAndSale::getExchangeCode() const noexcept {
    return data_.exchangeCode;
}

std::string TimeAndSale::getExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return utf16toUtf8String(data_.exchangeCode);
}

void TimeAndSale::setExchangeCode(std::int16_t exchangeCode) noexcept {
    data_.exchangeCode = exchangeCode;
}

double TimeAndSale::getPrice() const noexcept {
    return data_.price;
}

void TimeAndSale::setPrice(double price) noexcept {
    data_.price = price;
}

double TimeAndSale::getSize() const noexcept {
    return data_.size;
}

void TimeAndSale::setSize(double size) noexcept {
    data_.size = size;
}

double TimeAndSale::getBidPrice() const noexcept {
    return data_.bidPrice;
}

void TimeAndSale::setBidPrice(double bidPrice) noexcept {
    data_.bidPrice = bidPrice;
}

double TimeAndSale::getAskPrice() const noexcept {
    return data_.askPrice;
}

void TimeAndSale::setAskPrice(double askPrice) noexcept {
    data_.askPrice = askPrice;
}

const std::string &TimeAndSale::getExchangeSaleConditions() const & noexcept {
    if (!data_.exchangeSaleConditions) {
        return String::NUL;
    }

    return data_.exchangeSaleConditions.value();
}

const std::optional<std::string> &TimeAndSale::getExchangeSaleConditionsOpt() const & noexcept {
    return data_.exchangeSaleConditions;
}

void TimeAndSale::setExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept {
    data_.exchangeSaleConditions = std::string(exchangeSaleConditions);
}

char TimeAndSale::getTradeThroughExempt() const noexcept {
    return static_cast<char>(static_cast<unsigned char>(getBits(data_.flags, TTE_MASK, TTE_SHIFT)));
}

void TimeAndSale::setTradeThroughExempt(char tradeThroughExempt) {
    util::checkChar(tradeThroughExempt, TTE_MASK, "tradeThroughExempt");

    data_.flags = setBits(data_.flags, TTE_MASK, TTE_SHIFT, tradeThroughExempt);
}

const Side &TimeAndSale::getAggressorSide() const & noexcept {
    return Side::valueOf(getBits(data_.flags, SIDE_MASK, SIDE_SHIFT));
}

void TimeAndSale::setAggressorSide(const Side &side) noexcept {
    data_.flags = setBits(data_.flags, SIDE_MASK, SIDE_SHIFT, side.getCode());
}

bool TimeAndSale::isSpreadLeg() const noexcept {
    return andOp(data_.flags, SPREAD_LEG) != 0;
}

void TimeAndSale::setSpreadLeg(bool spreadLeg) noexcept {
    data_.flags = spreadLeg ? orOp(data_.flags, SPREAD_LEG) : andOp(data_.flags, ~SPREAD_LEG);
}

bool TimeAndSale::isExtendedTradingHours() const noexcept {
    return andOp(data_.flags, ETH) != 0;
}

void TimeAndSale::setExtendedTradingHours(bool extendedTradingHours) noexcept {
    data_.flags = extendedTradingHours ? orOp(data_.flags, ETH) : andOp(data_.flags, ~ETH);
}

bool TimeAndSale::isValidTick() const noexcept {
    return andOp(data_.flags, VALID_TICK) != 0;
}

void TimeAndSale::setValidTick(bool validTick) noexcept {
    data_.flags = validTick ? orOp(data_.flags, VALID_TICK) : andOp(data_.flags, ~VALID_TICK);
}

const TimeAndSaleType &TimeAndSale::getType() const & noexcept {
    return TimeAndSaleType::valueOf(getBits(data_.flags, TYPE_MASK, TYPE_SHIFT));
}

void TimeAndSale::setType(const TimeAndSaleType &type) noexcept {
    data_.flags = setBits(data_.flags, TYPE_MASK, TYPE_SHIFT, type.getCode());
}

bool TimeAndSale::isNew() const noexcept {
    return getType() == TimeAndSaleType::NEW;
}

bool TimeAndSale::isCorrection() const noexcept {
    return getType() == TimeAndSaleType::CORRECTION;
}

bool TimeAndSale::isCancel() const noexcept {
    return getType() == TimeAndSaleType::CANCEL;
}

const std::string &TimeAndSale::getBuyer() const & noexcept {
    if (!data_.buyer) {
        return String::NUL;
    }

    return data_.buyer.value();
}

const std::optional<std::string> &TimeAndSale::getBuyerOpt() const & noexcept {
    return data_.buyer;
}

void TimeAndSale::setBuyer(const StringLike &buyer) noexcept {
    data_.buyer = std::string(buyer);
}

const std::string &TimeAndSale::getSeller() const & noexcept {
    if (!data_.seller) {
        return String::NUL;
    }

    return data_.seller.value();
}

const std::optional<std::string> &TimeAndSale::getSellerOpt() const & noexcept {
    return data_.seller;
}

void TimeAndSale::setSeller(const StringLike &seller) noexcept {
    data_.seller = std::string(seller);
}

std::string TimeAndSale::toString() const {
    return fmt::format("TimeAndSale{{{}, eventTime={}, eventFlags={:#x}, time={}, timeNanoPart={}, sequence={}, "
                       "exchange={}, price={}, size={}, bid={}, ask={}, ESC='{}', TTE={}, side={}, spread={}, ETH={}, "
                       "validTick={}, type={}{}{}}}",
                       MarketEvent::getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
                       getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()),
                       getTimeNanoPart(), getSequence(), encodeChar(getExchangeCode()), dxfcpp::toString(getPrice()),
                       dxfcpp::toString(getSize()), dxfcpp::toString(getBidPrice()), dxfcpp::toString(getAskPrice()),
                       getExchangeSaleConditions(), encodeChar(getTradeThroughExempt()), getAggressorSide().toString(),
                       isSpreadLeg(), isExtendedTradingHours(), isValidTick(), getType().toString(),
                       getBuyer().empty() ? std::string{} : fmt::format(", buyer='{}'", getBuyer()),
                       getSeller().empty() ? std::string{} : fmt::format(", seller='{}'", getSeller()));
}

DXFCPP_END_NAMESPACE