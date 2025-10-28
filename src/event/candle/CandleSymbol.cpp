// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp>
#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

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

    auto *graalSymbol = static_cast<dxfg_candle_symbol_t *>(graalNative);

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

DXFCPP_END_NAMESPACE