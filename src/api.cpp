// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8cpp/utf8.h>
#include <utility>

namespace dxfcpp {

char utf16to8(std::int16_t in) {
    try {
        std::string out{};
        auto utf16in = {in};

        utf8::utf16to8(std::begin(utf16in), std::end(utf16in), std::back_inserter(out));

        return out.empty() ? char{} : out[0];
    } catch (...) {
        // TODO: error handling
        return char{};
    }
}

std::int16_t utf8to16(char in) {
    try {
        std::u16string out{};
        auto utf8in = {in};

        utf8::utf8to16(std::begin(utf8in), std::end(utf8in), std::back_inserter(out));

        return out.empty() ? std::int16_t{} : static_cast<std::int16_t>(out[0]);
    } catch (...) {
        // TODO: error handling
        return std::int16_t{};
    }
}

const EventTypeEnum &Quote::Type = EventTypeEnum::QUOTE;

std::int16_t Quote::getBidExchangeCode() const { return data_.bidExchangeCode; }

void Quote::setBidExchangeCode(char bidExchangeCode) { data_.bidExchangeCode = utf8to16(bidExchangeCode); }

void Quote::setBidExchangeCode(std::int16_t bidExchangeCode) { data_.bidExchangeCode = bidExchangeCode; }

std::int16_t Quote::getAskExchangeCode() const { return data_.askExchangeCode; }

void Quote::setAskExchangeCode(char askExchangeCode) { data_.askExchangeCode = utf8to16(askExchangeCode); }

void Quote::setAskExchangeCode(std::int16_t askExchangeCode) { data_.askExchangeCode = askExchangeCode; }

std::shared_ptr<Quote> Quote::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_QUOTE) {
        return {};
    }

    try {
        auto graalQuote = bit_cast<dxfg_quote_t *>(graalNative);
        auto quote = std::make_shared<Quote>(dxfcpp::toString(graalQuote->market_event.event_symbol));

        quote->setEventTime(graalQuote->market_event.event_time);

        quote->data_ = {
            graalQuote->time_millis_sequence,
            graalQuote->time_nano_part,
            graalQuote->bid_time,
            graalQuote->bid_exchange_code,
            graalQuote->bid_price,
            graalQuote->bid_size,
            graalQuote->ask_time,
            graalQuote->ask_exchange_code,
            graalQuote->ask_price,
            graalQuote->ask_size,
        };

        return quote;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const ShortSaleRestriction ShortSaleRestriction::UNDEFINED{0, "UNDEFINED"};
const ShortSaleRestriction ShortSaleRestriction::ACTIVE{1, "ACTIVE"};
const ShortSaleRestriction ShortSaleRestriction::INACTIVE{2, "INACTIVE"};

template <>
const std::unordered_map<ShortSaleRestriction::CodeType, std::reference_wrapper<const ShortSaleRestriction>>
    ShortSaleRestriction::ParentType::ALL{
        {ShortSaleRestriction::UNDEFINED.getCode(), std::cref(ShortSaleRestriction::UNDEFINED)},
        {ShortSaleRestriction::ACTIVE.getCode(), std::cref(ShortSaleRestriction::ACTIVE)},
        {ShortSaleRestriction::INACTIVE.getCode(), std::cref(ShortSaleRestriction::INACTIVE)},
    };

const TradingStatus TradingStatus::UNDEFINED{0, "UNDEFINED"};
const TradingStatus TradingStatus::HALTED{1, "HALTED"};
const TradingStatus TradingStatus::ACTIVE{2, "ACTIVE"};

template <>
const std::unordered_map<TradingStatus::CodeType, std::reference_wrapper<const TradingStatus>>
    TradingStatus::ParentType::ALL{
        {TradingStatus::UNDEFINED.getCode(), std::cref(TradingStatus::UNDEFINED)},
        {TradingStatus::HALTED.getCode(), std::cref(TradingStatus::HALTED)},
        {TradingStatus::ACTIVE.getCode(), std::cref(TradingStatus::ACTIVE)},
    };

const EventTypeEnum &Profile::Type = EventTypeEnum::PROFILE;

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

const PriceType PriceType::REGULAR{0, "REGULAR"};
const PriceType PriceType::INDICATIVE{1, "INDICATIVE"};
const PriceType PriceType::PRELIMINARY{2, "PRELIMINARY"};
const PriceType PriceType::FINAL{3, "FINAL"};

template <>
const std::unordered_map<PriceType::CodeType, std::reference_wrapper<const PriceType>> PriceType::ParentType::ALL{
    {PriceType::REGULAR.getCode(), std::cref(PriceType::REGULAR)},
    {PriceType::INDICATIVE.getCode(), std::cref(PriceType::INDICATIVE)},
    {PriceType::PRELIMINARY.getCode(), std::cref(PriceType::PRELIMINARY)},
    {PriceType::FINAL.getCode(), std::cref(PriceType::FINAL)},
};

const EventTypeEnum &Summary::Type = EventTypeEnum::SUMMARY;

std::shared_ptr<Summary> Summary::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_SUMMARY) {
        return {};
    }

    try {
        auto graalSummary = bit_cast<dxfg_summary_t *>(graalNative);
        auto summary = std::make_shared<Summary>(dxfcpp::toString(graalSummary->market_event.event_symbol));

        summary->setEventTime(graalSummary->market_event.event_time);
        summary->data_ = {graalSummary->day_id,
                          graalSummary->day_open_price,
                          graalSummary->day_high_price,
                          graalSummary->day_low_price,
                          graalSummary->day_close_price,
                          graalSummary->prev_day_id,
                          graalSummary->prev_day_close_price,
                          graalSummary->prev_day_volume,
                          graalSummary->open_interest,
                          graalSummary->flags};

        return summary;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const EventTypeEnum &Greeks::Type = EventTypeEnum::GREEKS;

std::shared_ptr<Greeks> Greeks::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_GREEKS) {
        return {};
    }

    try {
        auto graalGreeks = bit_cast<dxfg_greeks_t *>(graalNative);
        auto greeks = std::make_shared<Greeks>(dxfcpp::toString(graalGreeks->market_event.event_symbol));

        greeks->setEventTime(graalGreeks->market_event.event_time);

        greeks->data_ = {
            graalGreeks->event_flags, graalGreeks->index, graalGreeks->price,
            graalGreeks->volatility,  graalGreeks->delta, graalGreeks->gamma,
            graalGreeks->theta,       graalGreeks->rho,   graalGreeks->vega,
        };

        return greeks;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const EventTypeEnum &Underlying::Type = EventTypeEnum::UNDERLYING;

std::shared_ptr<Underlying> Underlying::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_UNDERLYING) {
        return {};
    }

    try {
        auto graalUnderlying = bit_cast<dxfg_underlying_t *>(graalNative);
        auto underlying = std::make_shared<Underlying>(dxfcpp::toString(graalUnderlying->market_event.event_symbol));

        underlying->setEventTime(graalUnderlying->market_event.event_time);
        underlying->data_ = {
            graalUnderlying->event_flags,      graalUnderlying->index,           graalUnderlying->volatility,
            graalUnderlying->front_volatility, graalUnderlying->back_volatility, graalUnderlying->call_volume,
            graalUnderlying->put_volume,       graalUnderlying->put_call_ratio,
        };

        return underlying;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const EventTypeEnum &TheoPrice::Type = EventTypeEnum::THEO_PRICE;

std::shared_ptr<TheoPrice> TheoPrice::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_THEO_PRICE) {
        return {};
    }

    try {
        auto graalTheoPrice = bit_cast<dxfg_theo_price_t *>(graalNative);
        auto theoPrice = std::make_shared<TheoPrice>(dxfcpp::toString(graalTheoPrice->market_event.event_symbol));

        theoPrice->setEventTime(graalTheoPrice->market_event.event_time);
        theoPrice->data_ = {
            graalTheoPrice->event_flags,      graalTheoPrice->index,    graalTheoPrice->price,
            graalTheoPrice->underlying_price, graalTheoPrice->delta,    graalTheoPrice->gamma,
            graalTheoPrice->dividend,         graalTheoPrice->interest,
        };

        return theoPrice;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const Direction Direction::UNDEFINED{0, "UNDEFINED"};
const Direction Direction::DOWN{1, "DOWN"};
const Direction Direction::ZERO_DOWN{2, "ZERO_DOWN"};
const Direction Direction::ZERO{3, "ZERO"};
const Direction Direction::ZERO_UP{4, "ZERO_UP"};
const Direction Direction::UP{5, "UP"};

template <>
const std::unordered_map<Direction::CodeType, std::reference_wrapper<const Direction>> Direction::ParentType::ALL{
    {Direction::UNDEFINED.getCode(), std::cref(Direction::UNDEFINED)},
    {Direction::DOWN.getCode(), std::cref(Direction::DOWN)},
    {Direction::ZERO_DOWN.getCode(), std::cref(Direction::ZERO_DOWN)},
    {Direction::ZERO.getCode(), std::cref(Direction::ZERO)},
    {Direction::ZERO_UP.getCode(), std::cref(Direction::ZERO_UP)},
    {Direction::UP.getCode(), std::cref(Direction::UP)},
};

void TradeBase::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

const EventTypeEnum &Trade::Type = EventTypeEnum::TRADE;

std::shared_ptr<Trade> Trade::fromGraalNative(void *graalNative) noexcept {
    return TradeBase::fromGraalNative<Trade, dxfg_event_type_t, dxfg_trade_t, dxfg_event_clazz_t::DXFG_EVENT_TRADE>(
        graalNative);
}

const EventTypeEnum &TradeETH::Type = EventTypeEnum::TRADE_ETH;

std::shared_ptr<TradeETH> TradeETH::fromGraalNative(void *graalNative) noexcept {
    return TradeBase::fromGraalNative<TradeETH, dxfg_event_type_t, dxfg_trade_eth_t,
                                      dxfg_event_clazz_t::DXFG_EVENT_TRADE_ETH>(graalNative);
}

const Side Side::UNDEFINED{0, "UNDEFINED"};
const Side Side::BUY{1, "BUY"};
const Side Side::SELL{2, "SELL"};

template <>
const std::unordered_map<Side::CodeType, std::reference_wrapper<const Side>> Side::ParentType::ALL{
    {Side::UNDEFINED.getCode(), std::cref(Side::UNDEFINED)},
    {Side::BUY.getCode(), std::cref(Side::BUY)},
    {Side::SELL.getCode(), std::cref(Side::SELL)},
};

const TimeAndSaleType TimeAndSaleType::NEW{0, "NEW"};
const TimeAndSaleType TimeAndSaleType::CORRECTION{1, "CORRECTION"};
const TimeAndSaleType TimeAndSaleType::CANCEL{2, "CANCEL"};

template <>
const std::unordered_map<TimeAndSaleType::CodeType, std::reference_wrapper<const TimeAndSaleType>>
    TimeAndSaleType::ParentType::ALL{
        {TimeAndSaleType::NEW.getCode(), std::cref(TimeAndSaleType::NEW)},
        {TimeAndSaleType::CORRECTION.getCode(), std::cref(TimeAndSaleType::CORRECTION)},
        {TimeAndSaleType::CANCEL.getCode(), std::cref(TimeAndSaleType::CANCEL)},
    };

void TimeAndSale::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

const EventTypeEnum &TimeAndSale::Type = EventTypeEnum::TIME_AND_SALE;

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_TIME_AND_SALE) {
        return {};
    }

    try {
        auto graalTimeAndSale = bit_cast<dxfg_time_and_sale_t *>(graalNative);
        auto timeAndSale = std::make_shared<TimeAndSale>(dxfcpp::toString(graalTimeAndSale->market_event.event_symbol));

        timeAndSale->setEventTime(graalTimeAndSale->market_event.event_time);
        timeAndSale->data_ = {
            graalTimeAndSale->event_flags,
            graalTimeAndSale->index,
            graalTimeAndSale->time_nano_part,
            graalTimeAndSale->exchange_code,
            graalTimeAndSale->price,
            graalTimeAndSale->size,
            graalTimeAndSale->bid_price,
            graalTimeAndSale->ask_price,
            dxfcpp::toString(graalTimeAndSale->exchange_sale_conditions),
            graalTimeAndSale->flags,
            dxfcpp::toString(graalTimeAndSale->buyer),
            dxfcpp::toString(graalTimeAndSale->seller),
        };

        return timeAndSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

const EventTypeEnum &Series::Type = EventTypeEnum::SERIES;

std::shared_ptr<Series> Series::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_SERIES) {
        return {};
    }

    try {
        auto graalSeries = bit_cast<dxfg_series_t *>(graalNative);
        auto series = std::make_shared<Series>(dxfcpp::toString(graalSeries->market_event.event_symbol));

        series->setEventTime(graalSeries->market_event.event_time);
        series->data_ = {
            graalSeries->event_flags, graalSeries->index,          graalSeries->time_sequence,
            graalSeries->expiration,  graalSeries->volatility,     graalSeries->call_volume,
            graalSeries->put_volume,  graalSeries->put_call_ratio, graalSeries->forward_price,
            graalSeries->dividend,    graalSeries->interest,
        };

        return series;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

//

const EventTypeEnum &OptionSale::Type = EventTypeEnum::OPTION_SALE;

std::shared_ptr<OptionSale> OptionSale::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_OPTION_SALE) {
        return {};
    }

    try {
        auto graalOptionSale = bit_cast<dxfg_option_sale_t *>(graalNative);
        auto optionSale = std::make_shared<OptionSale>(dxfcpp::toString(graalOptionSale->market_event.event_symbol));

        optionSale->setEventTime(graalOptionSale->market_event.event_time);
        optionSale->data_ = {
            graalOptionSale->event_flags,
            graalOptionSale->index,
            graalOptionSale->time_sequence,
            graalOptionSale->time_nano_part,
            graalOptionSale->exchange_code,
            graalOptionSale->price,
            graalOptionSale->size,
            graalOptionSale->bid_price,
            graalOptionSale->ask_price,
            dxfcpp::toString(graalOptionSale->exchange_sale_conditions),
            graalOptionSale->flags,
            graalOptionSale->underlying_price,
            graalOptionSale->volatility,
            graalOptionSale->delta,
            dxfcpp::toString(graalOptionSale->option_symbol),
        };

        return optionSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

void OptionSale::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalNativeList(void *graalNativeList) {
    auto list = bit_cast<dxfg_event_type_list *>(graalNativeList);

    if (list->size <= 0) {
        return {};
    }

    std::vector<std::shared_ptr<EventType>> result{static_cast<std::size_t>(list->size)};

    for (std::size_t i = 0; i < list->size; i++) {
        auto *e = list->elements[i];

        // TODO: implement other types
        switch (e->clazz) {
        case DXFG_EVENT_QUOTE:
            result[i] = Quote::fromGraalNative(e);

            break;
        case DXFG_EVENT_PROFILE:
            result[i] = Profile::fromGraalNative(e);

            break;
        case DXFG_EVENT_SUMMARY:
            result[i] = Summary::fromGraalNative(e);

            break;
        case DXFG_EVENT_GREEKS:
            result[i] = Greeks::fromGraalNative(e);

            break;
        case DXFG_EVENT_CANDLE:
            break;
        case DXFG_EVENT_DAILY_CANDLE:
            break;
        case DXFG_EVENT_UNDERLYING:
            result[i] = Underlying::fromGraalNative(e);

            break;
        case DXFG_EVENT_THEO_PRICE:
            result[i] = TheoPrice::fromGraalNative(e);

            break;
        case DXFG_EVENT_TRADE:
            result[i] = Trade::fromGraalNative(e);

            break;
        case DXFG_EVENT_TRADE_ETH:
            result[i] = TradeETH::fromGraalNative(e);

            break;
        case DXFG_EVENT_CONFIGURATION:
            break;
        case DXFG_EVENT_MESSAGE:
            break;
        case DXFG_EVENT_TIME_AND_SALE:
            result[i] = TimeAndSale::fromGraalNative(e);

            break;
        case DXFG_EVENT_ORDER_BASE:
            break;
        case DXFG_EVENT_ORDER:
            break;
        case DXFG_EVENT_ANALYTIC_ORDER:
            break;
        case DXFG_EVENT_SPREAD_ORDER:
            break;
        case DXFG_EVENT_SERIES:
            result[i] = Series::fromGraalNative(e);

            break;
        case DXFG_EVENT_OPTION_SALE:
            result[i] = OptionSale::fromGraalNative(e);

            break;
        }
    }

    return result;
}

} // namespace dxfcpp



namespace dxfcpp {

struct BuilderHandle {};

struct BuilderRegistry {
    static std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> builders;

    static std::shared_ptr<DXEndpoint::Builder> get(BuilderHandle *handle) {
        if (builders.contains(handle)) {
            return builders[handle];
        }

        return {};
    }

    static BuilderHandle *add(std::shared_ptr<DXEndpoint::Builder> builder) noexcept {
        auto handle = new (std::nothrow) BuilderHandle{};

        builders[handle] = std::move(builder);

        return handle;
    }

    static bool remove(BuilderHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = builders.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> BuilderRegistry::builders{};

struct EndpointWrapperHandle {};

struct EndpointWrapper : std::enable_shared_from_this<EndpointWrapper> {
    std::shared_ptr<dxfcpp::DXEndpoint> endpoint{};
    void *userData{};
    std::unordered_map<dxfc_dxendpoint_state_change_listener, std::size_t> listeners{};

    EndpointWrapper(std::shared_ptr<dxfcpp::DXEndpoint> endpoint, void *userData)
        : endpoint{std::move(endpoint)}, userData{userData}, listeners{} {}
};

struct EndpointWrapperRegistry {
    static std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>> endpointWrappers;

    static std::shared_ptr<EndpointWrapper> get(EndpointWrapperHandle *handle) {
        if (endpointWrappers.contains(handle)) {
            return endpointWrappers[handle];
        }

        return {};
    }

    static EndpointWrapperHandle *add(std::shared_ptr<EndpointWrapper> endpointWrapper) noexcept {
        auto handle = new (std::nothrow) EndpointWrapperHandle{};

        endpointWrappers[handle] = std::move(endpointWrapper);

        return handle;
    }

    static bool remove(EndpointWrapperHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = endpointWrappers.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>>
    EndpointWrapperRegistry::endpointWrappers{};

static dxfcpp::DXEndpoint::Role cApiRoleToRole(dxfc_dxendpoint_role_t role) {
    switch (role) {
    case DXFC_DXENDPOINT_ROLE_FEED:
        return dxfcpp::DXEndpoint::Role::FEED;
    case DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED:
        return dxfcpp::DXEndpoint::Role::ON_DEMAND_FEED;
    case DXFC_DXENDPOINT_ROLE_STREAM_FEED:
        return dxfcpp::DXEndpoint::Role::STREAM_FEED;
    case DXFC_DXENDPOINT_ROLE_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::STREAM_PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_LOCAL_HUB:
        return dxfcpp::DXEndpoint::Role::LOCAL_HUB;
    }

    return dxfcpp::DXEndpoint::Role::FEED;
}

static dxfc_dxendpoint_role_t roleToCApiRole(dxfcpp::DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFC_DXENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFC_DXENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFC_DXENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFC_DXENDPOINT_ROLE_FEED;
}

static dxfc_dxendpoint_state_t stateToCApiState(dxfcpp::DXEndpoint::State state) {
    switch (state) {
    case DXEndpoint::State::NOT_CONNECTED:
        return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
    case DXEndpoint::State::CONNECTING:
        return DXFC_DXENDPOINT_STATE_CONNECTING;
    case DXEndpoint::State::CONNECTED:
        return DXFC_DXENDPOINT_STATE_CONNECTED;
    case DXEndpoint::State::CLOSED:
        return DXFC_DXENDPOINT_STATE_CLOSED;
    }

    return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
}

} // namespace dxfcpp

dxfc_error_code_t dxfc_dxendpoint_new_builder(DXFC_OUT dxfc_dxendpoint_builder_t *builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::BuilderRegistry::add(dxfcpp::DXEndpoint::newBuilder());

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *builderHandle = dxfcpp::bit_cast<dxfc_dxendpoint_builder_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_role(dxfc_dxendpoint_builder_t builderHandle,
                                                    dxfc_dxendpoint_role_t role) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withRole(dxfcpp::cApiRoleToRole(role));

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_name(dxfc_dxendpoint_builder_t builderHandle, const char *name) {
    if (!builderHandle || !name) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withName(name);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                        const char *value) {
    if (!builderHandle || !key || !value) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withProperty(key, value);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_properties(dxfc_dxendpoint_builder_t builderHandle,
                                                          const dxfc_dxendpoint_property_t **properties, size_t size) {
    if (!builderHandle || !properties || size == 0) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    for (std::size_t i = 0; i < size; i++) {
        if (!properties[i]) {
            continue;
        }

        if (!properties[i]->key || !properties[i]->value) {
            continue;
        }

        builder->withProperty(properties[i]->key, properties[i]->value);
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_supports_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                            DXFC_OUT int *supports) {
    if (!builderHandle || !key || !supports) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    *supports = builder->supportsProperty(key);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_build(dxfc_dxendpoint_builder_t builderHandle, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!builderHandle || !endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = builder->build();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_free(dxfc_dxendpoint_builder_t builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::BuilderRegistry::remove(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance2(dxfc_dxendpoint_role_t role, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create2(dxfc_dxendpoint_role_t role, void *userData,
                                          DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->close();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close_and_await_termination(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->closeAndAwaitTermination();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_role(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_role_t *role) {
    if (!endpointHandle || !role) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *role = dxfcpp::roleToCApiRole(endpointWrapper->endpoint->getRole());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_user(dxfc_dxendpoint_t endpointHandle, const char *user) {
    if (!endpointHandle || !user) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->user(user);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_password(dxfc_dxendpoint_t endpointHandle, const char *password) {
    if (!endpointHandle || !password) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->password(password);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_connect(dxfc_dxendpoint_t endpointHandle, const char *address) {
    if (!endpointHandle || !address) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->connect(address);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_reconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->reconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect_and_clear(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnectAndClear();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_processed(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitProcessed();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_not_connected(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitNotConnected();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_state(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_state_t *state) {
    if (!endpointHandle || !state) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *state = dxfcpp::stateToCApiState(endpointWrapper->endpoint->getState());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_add_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                            dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->listeners[listener] = endpointWrapper->endpoint->onStateChange() +=
        [userData = endpointWrapper->userData, listener](dxfcpp::DXEndpoint::State oldState,
                                                         dxfcpp::DXEndpoint::State newState) {
            listener(dxfcpp::stateToCApiState(oldState), dxfcpp::stateToCApiState(newState), userData);
        };

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_remove_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                               dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (!endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->endpoint->onStateChange() -= endpointWrapper->listeners[listener];

    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_feed(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxfeed_t *feed) {
    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_publisher(dxfc_dxendpoint_t endpointHandle,
                                                DXFC_OUT dxfc_dxpublisher_t *publisher) {
    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_free(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::EndpointWrapperRegistry::remove(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}