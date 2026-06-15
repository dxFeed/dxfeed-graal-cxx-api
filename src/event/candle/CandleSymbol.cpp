// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/resources/Strings.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp"

#include <fmt/format.h>
#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

const CandleAlignment CandleAlignment::MIDNIGHT{"m"};
const CandleAlignment CandleAlignment::SESSION{"s"};
const CandleAlignment CandleAlignment::DEFAULT = MIDNIGHT;
const std::string CandleAlignment::ATTRIBUTE_KEY{"a"};

const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, StringHash, std::equal_to<>>
    CandleAlignment::BY_STRING{
            {MIDNIGHT.toString(), std::cref(MIDNIGHT)},
            {SESSION.toString(), std::cref(SESSION)},
        };

const std::vector<std::reference_wrapper<const CandleAlignment>> CandleAlignment::VALUES{
    std::cref(MIDNIGHT),
    std::cref(SESSION),
};

//------------

const CandleExchange CandleExchange::COMPOSITE{'\0'};

const CandleExchange CandleExchange::DEFAULT = COMPOSITE;

//------------

const CandleType CandleType::TICK{"TICK", "t", 0LL};
const CandleType CandleType::SECOND{"SECOND", "s", 1000LL};
const CandleType CandleType::MINUTE{"MINUTE", "m", 60LL * 1000LL};
const CandleType CandleType::HOUR{"HOUR", "h", 60LL * 60LL * 1000L};
const CandleType CandleType::DAY{"DAY", "d", 24LL * 60LL * 60LL * 1000L};
const CandleType CandleType::WEEK{"WEEK", "w", 7LL * 24LL * 60LL * 60LL * 1000LL};
const CandleType CandleType::MONTH("MONTH", "mo", 30LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::OPTEXP("OPTEXP", "o", 30LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::YEAR("YEAR", "y", 365LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::VOLUME("VOLUME", "v", 0LL);
const CandleType CandleType::PRICE("PRICE", "p", 0LL);
const CandleType CandleType::PRICE_MOMENTUM("PRICE_MOMENTUM", "pm", 0LL);
const CandleType CandleType::PRICE_RENKO("PRICE_RENKO", "pr", 0LL);

const std::unordered_map<std::string, std::reference_wrapper<const CandleType>, StringHash, std::equal_to<>>
    CandleType::BY_STRING{
        {TICK.toString(), std::cref(TICK)},
        {SECOND.toString(), std::cref(SECOND)},
        {MINUTE.toString(), std::cref(MINUTE)},
        {HOUR.toString(), std::cref(HOUR)},
        {DAY.toString(), std::cref(DAY)},
        {WEEK.toString(), std::cref(WEEK)},
        {MONTH.toString(), std::cref(MONTH)},
        {OPTEXP.toString(), std::cref(OPTEXP)},
        {YEAR.toString(), std::cref(YEAR)},
        {VOLUME.toString(), std::cref(VOLUME)},
        {PRICE.toString(), std::cref(PRICE)},
        {PRICE_MOMENTUM.toString(), std::cref(PRICE_MOMENTUM)},
        {PRICE_RENKO.toString(), std::cref(PRICE_RENKO)},
    };

const std::vector<std::reference_wrapper<const CandleType>> CandleType::VALUES{
    std::cref(TICK),  std::cref(SECOND),         std::cref(MINUTE),      std::cref(HOUR), std::cref(DAY),
    std::cref(WEEK),  std::cref(MONTH),          std::cref(OPTEXP),      std::cref(YEAR), std::cref(VOLUME),
    std::cref(PRICE), std::cref(PRICE_MOMENTUM), std::cref(PRICE_RENKO),
};

//------------

const CandlePeriod CandlePeriod::TICK{1, CandleType::TICK};
const CandlePeriod CandlePeriod::DAY{1, CandleType::DAY};
const CandlePeriod CandlePeriod::DEFAULT = TICK;
const std::string CandlePeriod::ATTRIBUTE_KEY{};

//------------

const CandlePrice CandlePrice::LAST{"last"};
const CandlePrice CandlePrice::BID{"bid"};
const CandlePrice CandlePrice::ASK{"ask"};
const CandlePrice CandlePrice::MARK{"mark"};
const CandlePrice CandlePrice::SETTLEMENT{"s"};
const CandlePrice CandlePrice::DEFAULT = LAST;

const std::string CandlePrice::ATTRIBUTE_KEY{"price"};

const std::unordered_map<std::string, std::reference_wrapper<const CandlePrice>, StringHash, std::equal_to<>>
    CandlePrice::BY_STRING{
        {LAST.toString(), std::cref(LAST)},
        {BID.toString(), std::cref(BID)},
        {ASK.toString(), std::cref(ASK)},
        {MARK.toString(), std::cref(MARK)},
        {SETTLEMENT.toString(), std::cref(SETTLEMENT)},
    };

const std::vector<std::reference_wrapper<const CandlePrice>> CandlePrice::VALUES{
    std::cref(LAST), std::cref(BID), std::cref(ASK), std::cref(MARK), std::cref(SETTLEMENT),
};

//------------

const CandlePriceLevel CandlePriceLevel::DEFAULT{math::NaN};

const std::string CandlePriceLevel::ATTRIBUTE_KEY{"pl"};

//------------

const SessionType SessionType::NO_TRADING{SessionTypeEnum::NO_TRADING, "NO_TRADING", false};
const SessionType SessionType::PRE_MARKET{SessionTypeEnum::PRE_MARKET, "PRE_MARKET", true};
const SessionType SessionType::REGULAR{SessionTypeEnum::REGULAR, "REGULAR", true};
const SessionType SessionType::AFTER_MARKET{SessionTypeEnum::AFTER_MARKET, "AFTER_MARKET", true};

//------------

const SessionFilter SessionFilter::ANY{SessionFilterEnum::ANY, "ANY", std::nullopt, std::nullopt};
const SessionFilter SessionFilter::TRADING{SessionFilterEnum::TRADING, "TRADING", std::nullopt, true};
const SessionFilter SessionFilter::NON_TRADING{SessionFilterEnum::NON_TRADING, "NON_TRADING", std::nullopt, false};

const SessionFilter SessionFilter::NO_TRADING{SessionFilterEnum::NO_TRADING, "NO_TRADING", SessionType::NO_TRADING,
                                              std::nullopt};
const SessionFilter SessionFilter::PRE_MARKET{SessionFilterEnum::PRE_MARKET, "PRE_MARKET", SessionType::PRE_MARKET,
                                              std::nullopt};
const SessionFilter SessionFilter::REGULAR{SessionFilterEnum::REGULAR, "REGULAR", SessionType::REGULAR, std::nullopt};
const SessionFilter SessionFilter::AFTER_MARKET{SessionFilterEnum::AFTER_MARKET, "AFTER_MARKET",
                                                SessionType::AFTER_MARKET, std::nullopt};

const CandleSession CandleSession::ANY{SessionFilter::ANY, "false"};
const CandleSession CandleSession::REGULAR{SessionFilter::REGULAR, "true"};
const CandleSession CandleSession::DEFAULT = ANY;
const std::string CandleSession::ATTRIBUTE_KEY = "tho";

const std::unordered_map<std::string, std::reference_wrapper<const CandleSession>> CandleSession::BY_STRING{
        {ANY.toString(), std::cref(ANY)},
        {REGULAR.toString(), std::cref(REGULAR)},
    };

const std::vector<std::reference_wrapper<const CandleSession>> CandleSession::VALUES{
    std::cref(ANY),
    std::cref(REGULAR),
};

//------------

const CandleSymbol CandleSymbol::NUL{"<null>"};

std::string CandleSymbol::changeAttribute(const StringLike &symbol,
                                          const CandleSymbolAttributeVariant &attribute) noexcept {
    return std::visit(
        [&symbol](auto &&a) {
            return a.changeAttributeForSymbol(symbol);
        },
        attribute);
}

std::string CandleSymbol::normalize(const StringLike &s) noexcept {
    auto symbol = CandlePrice::normalizeAttributeForSymbol(s);

    symbol = CandleSession::normalizeAttributeForSymbol(symbol);
    symbol = CandlePeriod::normalizeAttributeForSymbol(symbol);
    symbol = CandleAlignment::normalizeAttributeForSymbol(symbol);
    symbol = CandlePriceLevel::normalizeAttributeForSymbol(symbol);

    return symbol;
}

void CandleSymbol::initTransientFields(bool force) noexcept {
    baseSymbol_ = MarketEventSymbols::getBaseSymbol(symbol_);

    if (!exchange_ || force) {
        exchange_.emplace(CandleExchange::getAttributeForSymbol(symbol_));
    }

    if (!price_ || force) {
        price_ = CandlePrice::getAttributeForSymbol(symbol_);
    }

    if (!session_ || force) {
        session_ = CandleSession::getAttributeForSymbol(symbol_);
    }

    if (!period_ || force) {
        period_ = CandlePeriod::getAttributeForSymbol(symbol_);
    }

    if (!alignment_ || force) {
        alignment_ = CandleAlignment::getAttributeForSymbol(symbol_);
    }

    if (!priceLevel_ || force) {
        priceLevel_ = CandlePriceLevel::getAttributeForSymbol(symbol_);
    }
}

CandleSymbol::CandleSymbol(const StringLike &symbol) noexcept : symbol_{normalize(symbol)} {
    initTransientFields();
}

CandleSymbol::CandleSymbol(const StringLike &symbol, const CandleSymbolAttributeVariant &attribute) noexcept
    : symbol_{normalize(changeAttribute(symbol, attribute))} {
    // TODO: check attributes
    initTransientFields();
}

const std::string &CandleSymbol::getBaseSymbol() const & noexcept {
    return baseSymbol_;
}

const std::optional<CandleExchange> &CandleSymbol::getExchange() const & noexcept {
    return exchange_;
}

const std::optional<CandlePrice> &CandleSymbol::getPrice() const & noexcept {
    return price_;
}

const std::optional<CandleSession> &CandleSymbol::getSession() const & noexcept {
    return session_;
}

const std::optional<CandlePeriod> &CandleSymbol::getPeriod() const & noexcept {
    return period_;
}

const std::optional<CandleAlignment> &CandleSymbol::getAlignment() const & noexcept {
    return alignment_;
}

const std::optional<CandlePriceLevel> &CandleSymbol::getPriceLevel() const & noexcept {
    return priceLevel_;
}

const std::string &CandleSymbol::toString() const & noexcept {
    return symbol_;
}

bool CandleSymbol::operator==(const CandleSymbol &candleSymbol) const noexcept {
    return symbol_ == candleSymbol.symbol_;
}

bool CandleSymbol::operator<(const CandleSymbol &candleSymbol) const noexcept {
    return symbol_ < candleSymbol.symbol_;
}

CandleSymbol::CandleSymbol(const CandleSymbol &candleSymbol) noexcept {
    symbol_ = candleSymbol.symbol_;
    initTransientFields(true);
}

CandleSymbol::CandleSymbol(CandleSymbol &&candleSymbol) noexcept {
    symbol_ = std::move(candleSymbol.symbol_);
    initTransientFields(true);
}

CandleSymbol &CandleSymbol::operator=(const CandleSymbol &candleSymbol) noexcept {
    if (this == &candleSymbol) {
        return *this;
    }

    symbol_ = candleSymbol.symbol_;
    initTransientFields(true);

    return *this;
}

CandleSymbol &CandleSymbol::operator=(CandleSymbol &&candleSymbol) noexcept {
    if (this == &candleSymbol) {
        return *this;
    }

    symbol_ = std::move(candleSymbol.symbol_);
    initTransientFields(true);

    return *this;
}

CandleSymbol::CandleSymbol() noexcept {
}

CandleSymbol::~CandleSymbol() noexcept {
}

void *CandleSymbol::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("CandleSymbol::toGraal()");
    }

    auto *graalSymbol = new dxfg_candle_symbol_t{{CANDLE}, createCString(symbol_)};

    return graalSymbol;
}

void CandleSymbol::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("CandleSymbol::freeGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    const auto *graalSymbol = static_cast<dxfg_candle_symbol_t *>(graalNative);

    delete[] graalSymbol->symbol;
    delete graalSymbol;
}

CandleSymbol CandleSymbol::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("CandleSymbol::fromGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return {};
    }

    const auto *graalSymbol = static_cast<dxfg_candle_symbol_t *>(graalNative);

    return CandleSymbol{graalSymbol->symbol};
}

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol) noexcept {
    return CandleSymbol{symbol};
}

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol, const CandleSymbolAttributeVariant &attribute) noexcept {
    return CandleSymbol{symbol, attribute};
}

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol,
                                   std::initializer_list<CandleSymbolAttributeVariant> attributes) noexcept {
    return valueOf(symbol, attributes.begin(), attributes.end());
}

CandleSymbol literals::operator""_c(const char *string, size_t length) noexcept {
    return CandleSymbol::valueOf(std::string{string, length});
}
DXFCPP_END_NAMESPACE