// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cstring>
#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/event/market/OptionSale.hpp>
#include <dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/internal/TimeFormat.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &OptionSale::TYPE = EventTypeEnum::OPTION_SALE;

void OptionSale::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    data_ = {
        .eventFlags = graalOptionSale->event_flags,
        .index = graalOptionSale->index,
        .timeSequence = graalOptionSale->time_sequence,
        .timeNanoPart = graalOptionSale->time_nano_part,
        .exchangeCode = graalOptionSale->exchange_code,
        .price = graalOptionSale->price,
        .size = graalOptionSale->size,
        .bidPrice = graalOptionSale->bid_price,
        .askPrice = graalOptionSale->ask_price,
        .exchangeSaleConditions = toStringOpt(graalOptionSale->exchange_sale_conditions),
        .flags = graalOptionSale->flags,
        .underlyingPrice = graalOptionSale->underlying_price,
        .volatility = graalOptionSale->volatility,
        .delta = graalOptionSale->delta,
        .optionSymbol = toStringOpt(graalOptionSale->option_symbol),
    };
}

void OptionSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    graalOptionSale->market_event.event_type.clazz = DXFG_EVENT_OPTION_SALE;
    graalOptionSale->event_flags = data_.eventFlags;
    graalOptionSale->index = data_.index;
    graalOptionSale->time_sequence = data_.timeSequence;
    graalOptionSale->time_nano_part = data_.timeNanoPart;
    graalOptionSale->exchange_code = data_.exchangeCode;
    graalOptionSale->price = data_.price;
    graalOptionSale->size = data_.size;
    graalOptionSale->bid_price = data_.bidPrice;
    graalOptionSale->ask_price = data_.askPrice;
    graalOptionSale->exchange_sale_conditions = createCString(data_.exchangeSaleConditions);
    graalOptionSale->flags = data_.flags;
    graalOptionSale->underlying_price = data_.underlyingPrice;
    graalOptionSale->volatility = data_.volatility;
    graalOptionSale->delta = data_.delta;
    graalOptionSale->option_symbol = createCString(data_.optionSymbol);
}

void OptionSale::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    const auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    delete[] graalOptionSale->exchange_sale_conditions;
    delete[] graalOptionSale->option_symbol;
}

std::shared_ptr<OptionSale> OptionSale::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create OptionSale. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OPTION_SALE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create OptionSale. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_OPTION_SALE)));
    }

    auto optionSale = std::make_shared<OptionSale>();

    optionSale->fillData(graalNative);

    return optionSale;
}

void *OptionSale::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOptionSale = new dxfg_option_sale_t{};

    fillGraalData(graalOptionSale);

    return graalOptionSale;
}

void OptionSale::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OPTION_SALE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free OptionSale's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_OPTION_SALE)));
    }

    const auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOptionSale;
}

void OptionSale::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<OptionSale>(); other) {
        data_ = other->data_;
    }
}

OptionSale::OptionSale(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

OptionSale &OptionSale::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

OptionSale &OptionSale::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

const IndexedEventSource &OptionSale::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t OptionSale::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask OptionSale::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void OptionSale::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

OptionSale &OptionSale::withEventFlags(std::int32_t eventFlags) noexcept {
    OptionSale::setEventFlags(eventFlags);

    return *this;
}

void OptionSale::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

OptionSale &OptionSale::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    OptionSale::setEventFlags(eventFlags);

    return *this;
}

std::int64_t OptionSale::getIndex() const noexcept {
    return data_.index;
}

void OptionSale::setIndex(std::int64_t index) {
    data_.index = index;
}

OptionSale &OptionSale::withIndex(std::int64_t index) noexcept {
    OptionSale::setIndex(index);

    return *this;
}

std::int64_t OptionSale::getTimeSequence() const noexcept {
    return data_.timeSequence;
}

void OptionSale::setTimeSequence(std::int64_t timeSequence) noexcept {
    data_.timeSequence = timeSequence;
}

OptionSale &OptionSale::withTimeSequence(std::int64_t timeSequence) noexcept {
    setTimeSequence(timeSequence);

    return *this;
}

std::int64_t OptionSale::getTime() const noexcept {
    return sar(data_.timeSequence, SECONDS_SHIFT) * 1000 +
           andOp(sar(data_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void OptionSale::setTime(std::int64_t time) noexcept {
    data_.timeSequence =
        orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                  sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
             getSequence());
}

OptionSale &OptionSale::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

std::int64_t OptionSale::getTimeNanos() const noexcept {
    return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
}

void OptionSale::setTimeNanos(std::int64_t timeNanos) noexcept {
    setTime(time_nanos_util::getMillisFromNanos(timeNanos));
    data_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
}

OptionSale &OptionSale::withTimeNanos(std::int64_t timeNanos) noexcept {
    setTimeNanos(timeNanos);

    return *this;
}

void OptionSale::setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    data_.timeNanoPart = timeNanoPart;
}

OptionSale &OptionSale::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    setTimeNanoPart(timeNanoPart);

    return *this;
}

std::int32_t OptionSale::getTimeNanoPart() const noexcept {
    return data_.timeNanoPart;
}

std::int32_t OptionSale::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(data_.timeSequence, MAX_SEQUENCE));
}

void OptionSale::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    data_.timeSequence = orOp(andOp(data_.timeSequence, ~MAX_SEQUENCE), sequence);
}

OptionSale &OptionSale::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

std::int16_t OptionSale::getExchangeCode() const noexcept {
    return data_.exchangeCode;
}

std::string OptionSale::getExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return utf16toUtf8String(data_.exchangeCode);
}

void OptionSale::setExchangeCode(char exchangeCode) noexcept {
    data_.exchangeCode = utf8to16(exchangeCode);
}

OptionSale &OptionSale::withExchangeCode(char exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

void OptionSale::setExchangeCode(std::int16_t exchangeCode) noexcept {
    data_.exchangeCode = exchangeCode;
}

OptionSale &OptionSale::withExchangeCode(std::int16_t exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

double OptionSale::getPrice() const noexcept {
    return data_.price;
}

void OptionSale::setPrice(double price) noexcept {
    data_.price = price;
}

OptionSale &OptionSale::withPrice(double price) noexcept {
    setPrice(price);

    return *this;
}

double OptionSale::getSize() const noexcept {
    return data_.size;
}

void OptionSale::setSize(double size) noexcept {
    data_.size = size;
}

OptionSale &OptionSale::withSize(double size) noexcept {
    setSize(size);

    return *this;
}

double OptionSale::getBidPrice() const noexcept {
    return data_.bidPrice;
}

void OptionSale::setBidPrice(double bidPrice) noexcept {
    data_.bidPrice = bidPrice;
}

OptionSale &OptionSale::withBidPrice(double bidPrice) noexcept {
    setBidPrice(bidPrice);

    return *this;
}

double OptionSale::getAskPrice() const noexcept {
    return data_.askPrice;
}

void OptionSale::setAskPrice(double askPrice) noexcept {
    data_.askPrice = askPrice;
}

OptionSale &OptionSale::withAskPrice(double askPrice) noexcept {
    setAskPrice(askPrice);

    return *this;
}

const std::string &OptionSale::getExchangeSaleConditions() const & noexcept {
    if (!data_.exchangeSaleConditions) {
        return String::NUL;
    }

    return data_.exchangeSaleConditions.value();
}

const std::optional<std::string> &OptionSale::getExchangeSaleConditionsOpt() const & noexcept {
    return data_.exchangeSaleConditions;
}

void OptionSale::setExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept {
    data_.exchangeSaleConditions = std::string(exchangeSaleConditions);
}

OptionSale &OptionSale::withExchangeSaleConditions(const StringLike &exchangeSaleConditions) noexcept {
    setExchangeSaleConditions(exchangeSaleConditions);

    return *this;
}

char OptionSale::getTradeThroughExempt() const noexcept {
    return static_cast<char>(
        static_cast<unsigned char>(getBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT)));
}

void OptionSale::setTradeThroughExempt(char tradeThroughExempt) {
    util::checkChar(tradeThroughExempt, TimeAndSale::TTE_MASK, "tradeThroughExempt");

    data_.flags = setBits(data_.flags, TimeAndSale::TTE_MASK, TimeAndSale::TTE_SHIFT, tradeThroughExempt);
}

OptionSale &OptionSale::withTradeThroughExempt(char tradeThroughExempt) {
    setTradeThroughExempt(tradeThroughExempt);

    return *this;
}

const Side &OptionSale::getAggressorSide() const & noexcept {
    return Side::valueOf(getBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT));
}

void OptionSale::setAggressorSide(const Side &side) noexcept {
    data_.flags = setBits(data_.flags, TimeAndSale::SIDE_MASK, TimeAndSale::SIDE_SHIFT, side.getCode());
}

OptionSale &OptionSale::withAggressorSide(const Side &side) noexcept {
    setAggressorSide(side);

    return *this;
}

bool OptionSale::isSpreadLeg() const noexcept {
    return andOp(data_.flags, TimeAndSale::SPREAD_LEG) != 0;
}

void OptionSale::setSpreadLeg(bool spreadLeg) noexcept {
    data_.flags = spreadLeg ? orOp(data_.flags, TimeAndSale::SPREAD_LEG) : andOp(data_.flags, ~TimeAndSale::SPREAD_LEG);
}

OptionSale &OptionSale::withSpreadLeg(bool spreadLeg) noexcept {
    setSpreadLeg(spreadLeg);

    return *this;
}

bool OptionSale::isExtendedTradingHours() const noexcept {
    return andOp(data_.flags, TimeAndSale::ETH) != 0;
}

void OptionSale::setExtendedTradingHours(bool extendedTradingHours) noexcept {
    data_.flags = extendedTradingHours ? orOp(data_.flags, TimeAndSale::ETH) : andOp(data_.flags, ~TimeAndSale::ETH);
}

OptionSale &OptionSale::withExtendedTradingHours(bool extendedTradingHours) noexcept {
    setExtendedTradingHours(extendedTradingHours);

    return *this;
}

bool OptionSale::isValidTick() const noexcept {
    return andOp(data_.flags, TimeAndSale::VALID_TICK) != 0;
}

void OptionSale::setValidTick(bool validTick) noexcept {
    data_.flags = validTick ? orOp(data_.flags, TimeAndSale::VALID_TICK) : andOp(data_.flags, ~TimeAndSale::VALID_TICK);
}

OptionSale &OptionSale::withValidTick(bool validTick) noexcept {
    setValidTick(validTick);

    return *this;
}

const TimeAndSaleType &OptionSale::getType() const & noexcept {
    return TimeAndSaleType::valueOf(getBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT));
}

void OptionSale::setType(const TimeAndSaleType &type) noexcept {
    data_.flags = setBits(data_.flags, TimeAndSale::TYPE_MASK, TimeAndSale::TYPE_SHIFT, type.getCode());
}

OptionSale &OptionSale::withType(const TimeAndSaleType &type) noexcept {
    setType(type);

    return *this;
}

bool OptionSale::isNew() const noexcept {
    return getType() == TimeAndSaleType::NEW;
}

bool OptionSale::isCorrection() const noexcept {
    return getType() == TimeAndSaleType::CORRECTION;
}

bool OptionSale::isCancel() const noexcept {
    return getType() == TimeAndSaleType::CANCEL;
}

double OptionSale::getUnderlyingPrice() const noexcept {
    return data_.underlyingPrice;
}

void OptionSale::setUnderlyingPrice(double underlyingPrice) noexcept {
    data_.underlyingPrice = underlyingPrice;
}

OptionSale &OptionSale::withUnderlyingPrice(double underlyingPrice) noexcept {
    setUnderlyingPrice(underlyingPrice);

    return *this;
}

double OptionSale::getVolatility() const noexcept {
    return data_.volatility;
}

void OptionSale::setVolatility(double volatility) noexcept {
    data_.volatility = volatility;
}

OptionSale &OptionSale::withVolatility(double volatility) noexcept {
    setVolatility(volatility);

    return *this;
}

double OptionSale::getDelta() const noexcept {
    return data_.delta;
}

void OptionSale::setDelta(double delta) noexcept {
    data_.delta = delta;
}

OptionSale &OptionSale::withDelta(double delta) noexcept {
    setDelta(delta);

    return *this;
}

const std::string &OptionSale::getOptionSymbol() const & noexcept {
    if (!data_.optionSymbol) {
        return String::NUL;
    }

    return data_.optionSymbol.value();
}

const std::optional<std::string> &OptionSale::getOptionSymbolOpt() const & noexcept {
    return data_.optionSymbol;
}

void OptionSale::setOptionSymbol(const StringLike &optionSymbol) noexcept {
    data_.optionSymbol = std::string(optionSymbol);
}

OptionSale &OptionSale::withOptionSymbol(const StringLike &optionSymbol) noexcept {
    setOptionSymbol(optionSymbol);

    return *this;
}

std::string OptionSale::toString() const {
    return fmt::format(
        "OptionSale{{{}, eventTime={}, eventFlags={:#x}, index={:#x}, time={}, timeNanoPart={}, sequence={}, "
        "exchange={}, price={}, size={}, bid={}, ask={}, ESC='{}', TTE={}, side={}, spread={}, ETH={}, "
        "validTick={}, type={}, underlyingPrice={}, volatility={}, delta={}, optionSymbol='{}'}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), getIndex(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getTimeNanoPart(),
        getSequence(), encodeChar(getExchangeCode()), dxfcpp::toString(getPrice()), dxfcpp::toString(getSize()),
        dxfcpp::toString(getBidPrice()), dxfcpp::toString(getAskPrice()), getExchangeSaleConditions(),
        encodeChar(getTradeThroughExempt()), getAggressorSide().toString(), isSpreadLeg(), isExtendedTradingHours(),
        isValidTick(), getType().toString(), dxfcpp::toString(getUnderlyingPrice()), dxfcpp::toString(getVolatility()),
        dxfcpp::toString(getDelta()), getOptionSymbol());
}

DXFCPP_END_NAMESPACE