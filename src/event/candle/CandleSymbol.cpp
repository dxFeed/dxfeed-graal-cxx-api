// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp"
#include "../../internal/resources/Strings.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"
#include "../../internal/utils/debug/Debug.hpp"

#include "../../internal/CandleSymbolHints.hpp"
#include "../../internal/utils/CandleUtils.hpp"
#include "../../internal/Common.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include <fmt/format.h>
#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

// The constants are constant-initialized (constinit), so they can be used in the dynamic initialization of global
// variables of other translation units. A constinit constant is not read in a constant expression, so DEFAULT and the
// other constants equal to a constant repeat its values, and the keys of the BY_STRING maps repeat the strings of the
// constants (ConstantsTest checks them).

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

namespace {
constexpr auto CANDLE_ALIGNMENTS_BY_STRING = ConstantMap<std::string_view, CandleAlignment>::sorted({
    {"m", CandleAlignment::MIDNIGHT},
    {"s", CandleAlignment::SESSION},
});
static_assert(CANDLE_ALIGNMENTS_BY_STRING.size() == std::tuple_size_v<decltype(CANDLE_ALIGNMENTS)>);
} // namespace

constinit const ConstantMap<std::string_view, CandleAlignment> CandleAlignment::BY_STRING{CANDLE_ALIGNMENTS_BY_STRING};

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

namespace {
constexpr auto CANDLE_TYPES_BY_STRING = ConstantMap<std::string_view, CandleType>::sorted({
    {"t", CandleType::TICK},
    {"s", CandleType::SECOND},
    {"m", CandleType::MINUTE},
    {"h", CandleType::HOUR},
    {"d", CandleType::DAY},
    {"w", CandleType::WEEK},
    {"mo", CandleType::MONTH},
    {"o", CandleType::OPTEXP},
    {"y", CandleType::YEAR},
    {"v", CandleType::VOLUME},
    {"p", CandleType::PRICE},
    {"pm", CandleType::PRICE_MOMENTUM},
    {"pr", CandleType::PRICE_RENKO},
});
static_assert(CANDLE_TYPES_BY_STRING.size() == std::tuple_size_v<decltype(CANDLE_TYPES)>);
} // namespace

constinit const ConstantMap<std::string_view, CandleType> CandleType::BY_STRING{CANDLE_TYPES_BY_STRING};

//------------

constinit const CandlePeriod CandlePeriod::TICK{1.0, "TICK", "t", 0LL};
constinit const CandlePeriod CandlePeriod::DAY{1.0, "DAY", "d", 24LL * 60LL * 60LL * 1000L};
constinit const CandlePeriod CandlePeriod::DEFAULT{1.0, "TICK", "t", 0LL};

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

namespace {
constexpr auto CANDLE_PRICES_BY_STRING = ConstantMap<std::string_view, CandlePrice>::sorted({
    {"last", CandlePrice::LAST},
    {"bid", CandlePrice::BID},
    {"ask", CandlePrice::ASK},
    {"mark", CandlePrice::MARK},
    {"s", CandlePrice::SETTLEMENT},
});
static_assert(CANDLE_PRICES_BY_STRING.size() == std::tuple_size_v<decltype(CANDLE_PRICES)>);
} // namespace

constinit const ConstantMap<std::string_view, CandlePrice> CandlePrice::BY_STRING{CANDLE_PRICES_BY_STRING};

//------------

constinit const CandlePriceLevel CandlePriceLevel::DEFAULT{std::numeric_limits<double>::quiet_NaN()};

//------------

constinit const CandleDataType CandleDataType::SOURCE{"source"};
constinit const CandleDataType CandleDataType::THO{"tho"};
constinit const CandleDataType CandleDataType::ALL{"all"};
constinit const CandleDataType CandleDataType::RTH{"rth"};
constinit const CandleDataType CandleDataType::ETH{"eth"};
constinit const CandleDataType CandleDataType::UNDEFINED{""};
constinit const CandleDataType CandleDataType::DEFAULT{""};

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

namespace {
constexpr auto CANDLE_SESSIONS_BY_STRING = ConstantMap<std::string_view, CandleSession>::sorted({
    {"false", CandleSession::ANY},
    {"true", CandleSession::REGULAR},
});
static_assert(CANDLE_SESSIONS_BY_STRING.size() == std::tuple_size_v<decltype(CANDLE_SESSIONS)>);
} // namespace

constinit const ConstantMap<std::string_view, CandleSession> CandleSession::BY_STRING{CANDLE_SESSIONS_BY_STRING};

//------------

std::string CandleSymbol::changeAttribute(StringParam symbol, const CandleSymbolAttributeVariant &attribute) {
    return std::visit(
        [&symbol](auto &&a) {
            return a.changeAttributeForSymbol(symbol);
        },
        attribute);
}

std::string CandleSymbol::normalize(StringParam symbol) {
    auto result = CandlePrice::normalizeAttributeForSymbol(symbol);

    result = CandleSession::normalizeAttributeForSymbol(result);
    result = CandlePeriod::normalizeAttributeForSymbol(result);
    result = CandleAlignment::normalizeAttributeForSymbol(result);
    result = CandlePriceLevel::normalizeAttributeForSymbol(result);
    result = CandleDataType::normalizeAttributeForSymbol(result);

    return result;
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

    if (!dataType_ || force) {
        dataType_ = CandleDataType::getAttributeForSymbol(symbol_);
    }
}

CandleSymbol::CandleSymbol(StringParam symbol) : symbol_{normalize(symbol)} {
    initTransientFields();
}

CandleSymbol::CandleSymbol(StringParam symbol, const CandleSymbolAttributeVariant &attribute)
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

const std::optional<CandleDataType> &CandleSymbol::getDataType() const & noexcept {
    return dataType_;
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

    auto *graalSymbol = new dxfg_candle_symbol_t{{CANDLE}, createCString(symbol_, "CandleSymbol(symbol)")};

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

namespace {

constexpr auto CANDLE_SYMBOL_HINTS_COMPONENT = "CandleSymbol";

// The number of the candle symbols that get a hint; the last of them gets one more warning that the hints are over.
constexpr std::size_t MAX_CANDLE_SYMBOL_HINTS = 1000;

// Set by detail::CandleSymbolHintsOff.
constinit thread_local bool candleSymbolHintsOff = false;

// The candle symbols that got a hint.
struct HintedCandleSymbols {
    std::mutex mutex{};
    std::unordered_set<std::string> symbols{};
};

// Never destroyed: a candle symbol can be created at the static initialization and destruction of the program.
HintedCandleSymbols &hintedCandleSymbols() {
    static auto *const instance = new HintedCandleSymbols{};

    return *instance;
}

// Returns the type of a candle period that the user likely meant instead of the parsed one, or an empty optional: the
// type `M` is a minute while a month is `mo`, the type `PR` is a price (the prefix of PRICE) while `pr` is PRICE_RENKO.
std::optional<std::reference_wrapper<const CandleType>> findLikelyMeantCandleType(std::string_view period,
                                                                                  const CandleType &type) {
    // The type follows the value, as in CandlePeriod::parse().
    const auto typeStart = period.find_first_not_of("0123456789.+-eE");

    if (typeStart == std::string_view::npos) {
        return std::nullopt;
    }

    const auto typeString = period.substr(typeStart);

    if (typeString == "M") {
        return std::cref(CandleType::MONTH);
    }

    for (const auto &other : CandleType::VALUES) {
        if (other.get() != type && iEquals(typeString, other.get().toString())) {
            return other;
        }
    }

    return std::nullopt;
}

// Returns the hint about a likely mistake in the string of a candle symbol, or an empty optional.
std::optional<std::string> findCandleSymbolHint(std::string_view string, const CandleSymbol &candleSymbol) {
    if (string.empty()) {
        return std::nullopt;
    }

    const auto period = MarketEventSymbols::getAttributeStringByKey(string, CandlePeriod::ATTRIBUTE_KEY);

    if (!period) {
        // A symbol of the price levels of ORCS has a granularity ("gr") instead of a period.
        if (MarketEventSymbols::getAttributeStringByKey(string, "gr")) {
            return std::nullopt;
        }

        const auto withPeriod = [&string](const char *value) {
            return MarketEventSymbols::changeAttributeStringByKey(string, CandlePeriod::ATTRIBUTE_KEY, value);
        };

        return fmt::format("Candle symbol '{}' has no period and gives tick candles; for other candles add a period, "
                           "for example '{}' or '{}', for tick candles write '{}'",
                           string, withPeriod("1m"), withPeriod("d"), withPeriod("t"));
    }

    const auto &parsedPeriod = candleSymbol.getPeriod();

    if (!parsedPeriod) {
        return std::nullopt;
    }

    const auto meantType = findLikelyMeantCandleType(*period, parsedPeriod->getType());

    if (!meantType) {
        return std::nullopt;
    }

    return fmt::format("Candle symbol '{}': the period '{}' means {} ('{}'); for {} write '{}'", string, *period,
                       parsedPeriod->getType().getName(), parsedPeriod->toString(), meantType->get().getName(),
                       CandlePeriod::valueOf(parsedPeriod->getValue(), meantType->get()).toString());
}

// Reports the hint about the string of a candle symbol as a warning of ErrorManager, once per string, for the first
// MAX_CANDLE_SYMBOL_HINTS strings.
void reportCandleSymbolHint(std::string_view string, const CandleSymbol &candleSymbol) {
    const auto hint = findCandleSymbolHint(string, candleSymbol);

    if (!hint) {
        return;
    }

    bool isLast = false;

    {
        auto &hinted = hintedCandleSymbols();
        std::lock_guard lock(hinted.mutex);

        if (hinted.symbols.size() >= MAX_CANDLE_SYMBOL_HINTS || !hinted.symbols.emplace(string).second) {
            return;
        }

        isLast = hinted.symbols.size() == MAX_CANDLE_SYMBOL_HINTS;
    }

    // Out of the lock: the handler of ErrorManager may create a candle symbol itself.
    ErrorManager::reportWarning(CANDLE_SYMBOL_HINTS_COMPONENT, *hint);

    if (isLast) {
        ErrorManager::reportWarning(CANDLE_SYMBOL_HINTS_COMPONENT,
                                    fmt::format("Hints were reported for {} candle symbols; the next candle symbols "
                                                "get no hints",
                                                MAX_CANDLE_SYMBOL_HINTS));
    }
}

} // namespace

// Doxygen skips src/internal, where this class is declared, and would warn on these definitions.
/// @cond
detail::CandleSymbolHintsOff::CandleSymbolHintsOff() noexcept : previous_{candleSymbolHintsOff} {
    candleSymbolHintsOff = true;
}

detail::CandleSymbolHintsOff::~CandleSymbolHintsOff() noexcept {
    candleSymbolHintsOff = previous_;
}
/// @endcond

CandleSymbol CandleSymbol::valueOf(StringParam symbol) {
    // A string that cannot be parsed throws here, before the hints.
    auto result = CandleSymbol{symbol};

    if (!candleSymbolHintsOff) {
        try {
            reportCandleSymbolHint(symbol.view(), result);
        } catch (...) {
            // A hint must not fail the creation of a valid symbol (for example, when the memory is exhausted).
        }
    }

    return result;
}

CandleSymbol CandleSymbol::valueOf(StringParam symbol, const CandleSymbolAttributeVariant &attribute) {
    return CandleSymbol{symbol, attribute};
}

CandleSymbol CandleSymbol::valueOf(StringParam symbol, std::initializer_list<CandleSymbolAttributeVariant> attributes) {
    return valueOf(symbol, attributes.begin(), attributes.end());
}

CandleSymbol literals::operator""_c(const char *string, size_t length) {
    return CandleSymbol::valueOf(std::string{string, length});
}
CandleSymbol CandleSymbol::NUL() {
    return CandleSymbol{String::NUL};
}

DXFCPP_END_NAMESPACE
