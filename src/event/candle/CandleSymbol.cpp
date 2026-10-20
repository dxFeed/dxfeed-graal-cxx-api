// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/resources/Strings.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp"

#include "../../internal/utils/CandleUtils.hpp"

#include <array>
#include <functional>
#include <limits>
#include <span>

#include <fmt/format.h>
#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

// The constants are constant-initialized (constinit), so they can be used in the dynamic initialization of global
// variables of other translation units. A constinit constant is not read in a constant expression, so DEFAULT and the
// other constants equal to a constant repeat its values. The maps and the keys are created at the first call of
// BY_STRING() and ATTRIBUTE_KEY().

constinit const CandleAlignment CandleAlignment::MIDNIGHT{"m"};
constinit const CandleAlignment CandleAlignment::SESSION{"s"};
constinit const CandleAlignment CandleAlignment::DEFAULT{"m"};

namespace {
constinit const std::array CANDLE_ALIGNMENTS{
    std::cref(CandleAlignment::MIDNIGHT),
    std::cref(CandleAlignment::SESSION),
};
} // namespace

constinit const std::span<const std::reference_wrapper<const CandleAlignment>> CandleAlignment::VALUES{
    CANDLE_ALIGNMENTS};

//------------

constinit const CandleExchange CandleExchange::COMPOSITE{'\0'};

constinit const CandleExchange CandleExchange::DEFAULT{'\0'};

//------------

constinit const CandleType CandleType::TICK{"TICK", "t", 0LL};
constinit const CandleType CandleType::SECOND{"SECOND", "s", 1000LL};
constinit const CandleType CandleType::MINUTE{"MINUTE", "m", 60LL * 1000LL};
constinit const CandleType CandleType::HOUR{"HOUR", "h", 60LL * 60LL * 1000L};
constinit const CandleType CandleType::DAY{"DAY", "d", 24LL * 60LL * 60LL * 1000L};
constinit const CandleType CandleType::WEEK{"WEEK", "w", 7LL * 24LL * 60LL * 60LL * 1000LL};
constinit const CandleType CandleType::MONTH("MONTH", "mo", 30LL * 24LL * 60LL * 60LL * 1000LL);
constinit const CandleType CandleType::OPTEXP("OPTEXP", "o", 30LL * 24LL * 60LL * 60LL * 1000LL);
constinit const CandleType CandleType::YEAR("YEAR", "y", 365LL * 24LL * 60LL * 60LL * 1000LL);
constinit const CandleType CandleType::VOLUME("VOLUME", "v", 0LL);
constinit const CandleType CandleType::PRICE("PRICE", "p", 0LL);
constinit const CandleType CandleType::PRICE_MOMENTUM("PRICE_MOMENTUM", "pm", 0LL);
constinit const CandleType CandleType::PRICE_RENKO("PRICE_RENKO", "pr", 0LL);

namespace {
constinit const std::array CANDLE_TYPES{
    std::cref(CandleType::TICK),        std::cref(CandleType::SECOND), std::cref(CandleType::MINUTE),
    std::cref(CandleType::HOUR),        std::cref(CandleType::DAY),    std::cref(CandleType::WEEK),
    std::cref(CandleType::MONTH),       std::cref(CandleType::OPTEXP), std::cref(CandleType::YEAR),
    std::cref(CandleType::VOLUME),      std::cref(CandleType::PRICE),  std::cref(CandleType::PRICE_MOMENTUM),
    std::cref(CandleType::PRICE_RENKO),
};
} // namespace

constinit const std::span<const std::reference_wrapper<const CandleType>> CandleType::VALUES{CANDLE_TYPES};

//------------

constinit const CandlePeriod CandlePeriod::TICK{1.0, "TICK", "t", 0LL, ConstString::Literal{"t"}};
constinit const CandlePeriod CandlePeriod::DAY{1.0, "DAY", "d", 24LL * 60LL * 60LL * 1000L, ConstString::Literal{"d"}};
constinit const CandlePeriod CandlePeriod::DEFAULT{1.0, "TICK", "t", 0LL, ConstString::Literal{"t"}};

//------------

constinit const CandlePrice CandlePrice::LAST{"last"};
constinit const CandlePrice CandlePrice::BID{"bid"};
constinit const CandlePrice CandlePrice::ASK{"ask"};
constinit const CandlePrice CandlePrice::MARK{"mark"};
constinit const CandlePrice CandlePrice::SETTLEMENT{"s"};
constinit const CandlePrice CandlePrice::DEFAULT{"last"};

namespace {
constinit const std::array CANDLE_PRICES{
    std::cref(CandlePrice::LAST), std::cref(CandlePrice::BID),        std::cref(CandlePrice::ASK),
    std::cref(CandlePrice::MARK), std::cref(CandlePrice::SETTLEMENT),
};
} // namespace

constinit const std::span<const std::reference_wrapper<const CandlePrice>> CandlePrice::VALUES{CANDLE_PRICES};

//------------

constinit const CandlePriceLevel CandlePriceLevel::DEFAULT{std::numeric_limits<double>::quiet_NaN()};

//------------

constinit const SessionType SessionType::NO_TRADING{SessionTypeEnum::NO_TRADING, "NO_TRADING", false};
constinit const SessionType SessionType::PRE_MARKET{SessionTypeEnum::PRE_MARKET, "PRE_MARKET", true};
constinit const SessionType SessionType::REGULAR{SessionTypeEnum::REGULAR, "REGULAR", true};
constinit const SessionType SessionType::AFTER_MARKET{SessionTypeEnum::AFTER_MARKET, "AFTER_MARKET", true};

//------------

constinit const SessionFilter SessionFilter::ANY{SessionFilterEnum::ANY, ConstString::Literal{"ANY"}, std::nullopt};
constinit const SessionFilter SessionFilter::TRADING{SessionFilterEnum::TRADING, ConstString::Literal{"TRADING"}, true};
constinit const SessionFilter SessionFilter::NON_TRADING{SessionFilterEnum::NON_TRADING,
                                                         ConstString::Literal{"NON_TRADING"}, false};

constinit const SessionFilter SessionFilter::NO_TRADING{SessionFilterEnum::NO_TRADING,
                                                        ConstString::Literal{"NO_TRADING"}, SessionTypeEnum::NO_TRADING,
                                                        "NO_TRADING", false};
constinit const SessionFilter SessionFilter::PRE_MARKET{
    SessionFilterEnum::PRE_MARKET, ConstString::Literal{"PRE_MARKET"}, SessionTypeEnum::PRE_MARKET, "PRE_MARKET", true};
constinit const SessionFilter SessionFilter::REGULAR{SessionFilterEnum::REGULAR, ConstString::Literal{"REGULAR"},
                                                     SessionTypeEnum::REGULAR, "REGULAR", true};
constinit const SessionFilter SessionFilter::AFTER_MARKET{SessionFilterEnum::AFTER_MARKET,
                                                          ConstString::Literal{"AFTER_MARKET"},
                                                          SessionTypeEnum::AFTER_MARKET, "AFTER_MARKET", true};

constinit const CandleSession CandleSession::ANY{SessionFilter::ANY, "false"};
constinit const CandleSession CandleSession::REGULAR{SessionFilter::REGULAR, "true"};
constinit const CandleSession CandleSession::DEFAULT{SessionFilter::ANY, "false"};

namespace {
constinit const std::array CANDLE_SESSIONS{
    std::cref(CandleSession::ANY),
    std::cref(CandleSession::REGULAR),
};
} // namespace

constinit const std::span<const std::reference_wrapper<const CandleSession>> CandleSession::VALUES{CANDLE_SESSIONS};

//------------

std::string CandleSymbol::changeAttribute(const StringLike &symbol, const CandleSymbolAttributeVariant &attribute) {
    return std::visit(
        [&symbol](auto &&a) {
            return a.changeAttributeForSymbol(symbol);
        },
        attribute);
}

std::string CandleSymbol::normalize(const StringLike &s) {
    auto symbol = CandlePrice::normalizeAttributeForSymbol(s);

    symbol = CandleSession::normalizeAttributeForSymbol(symbol);
    symbol = CandlePeriod::normalizeAttributeForSymbol(symbol);
    symbol = CandleAlignment::normalizeAttributeForSymbol(symbol);
    symbol = CandlePriceLevel::normalizeAttributeForSymbol(symbol);

    return symbol;
}

void CandleSymbol::initTransientFields(bool force) {
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

CandleSymbol::CandleSymbol(const StringLike &symbol) : symbol_{normalize(symbol)} {
    initTransientFields();
}

CandleSymbol::CandleSymbol(const StringLike &symbol, const CandleSymbolAttributeVariant &attribute)
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

CandleSymbol::CandleSymbol(const CandleSymbol &candleSymbol) = default;

CandleSymbol::CandleSymbol(CandleSymbol &&candleSymbol) = default;

CandleSymbol &CandleSymbol::operator=(const CandleSymbol &candleSymbol) = default;

CandleSymbol &CandleSymbol::operator=(CandleSymbol &&candleSymbol) = default;

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

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol) {
    return CandleSymbol{symbol};
}

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol, const CandleSymbolAttributeVariant &attribute) {
    return CandleSymbol{symbol, attribute};
}

CandleSymbol CandleSymbol::valueOf(const StringLike &symbol,
                                   std::initializer_list<CandleSymbolAttributeVariant> attributes) {
    return valueOf(symbol, attributes.begin(), attributes.end());
}

CandleSymbol literals::operator""_c(const char *string, size_t length) {
    return CandleSymbol::valueOf(std::string{string, length});
}
const CandleSymbol &CandleSymbol::NUL() {
    static const auto *const value = new CandleSymbol{"<null>"};

    return *value;
}

DXFCPP_END_NAMESPACE
