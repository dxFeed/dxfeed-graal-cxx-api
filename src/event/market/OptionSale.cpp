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

const EventTypeEnum &OptionSale::TYPE = EventTypeEnum::OPTION_SALE;

void OptionSale::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

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
        .exchangeSaleConditions = dxfcpp::toString(graalOptionSale->exchange_sale_conditions),
        .flags = graalOptionSale->flags,
        .underlyingPrice = graalOptionSale->underlying_price,
        .volatility = graalOptionSale->volatility,
        .delta = graalOptionSale->delta,
        .optionSymbol = dxfcpp::toString(graalOptionSale->option_symbol),
    };
}

void OptionSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

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

    auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    delete[] graalOptionSale->exchange_sale_conditions;
    delete[] graalOptionSale->option_symbol;
}

std::shared_ptr<OptionSale> OptionSale::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_OPTION_SALE) {
        return {};
    }

    try {
        auto optionSale = std::make_shared<OptionSale>();

        optionSale->fillData(graalNative);

        return optionSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

void OptionSale::setExchangeCode(char exchangeCode) noexcept {
    data_.exchangeCode = utf8to16(exchangeCode);
}

std::string OptionSale::toString() const noexcept {
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

void *OptionSale::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOptionSale = new (std::nothrow)
        dxfg_option_sale_t{.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_OPTION_SALE}}};

    if (!graalOptionSale) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalOptionSale));

    return static_cast<void *>(graalOptionSale);
}

void OptionSale::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_OPTION_SALE) {
        return;
    }

    auto graalOptionSale = static_cast<dxfg_option_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOptionSale;
}

} // namespace dxfcpp