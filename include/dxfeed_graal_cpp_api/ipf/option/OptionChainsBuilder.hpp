// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "OptionChain.hpp"
#include "OptionSeries.hpp"

#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Builder class for a set of option chains grouped by product or underlying symbol.
 *
 * <h3>Threads and clocks</h3>
 *
 * This class is <b>NOT</b> thread-safe and cannot be used from multiple threads without external synchronization.
 *
 * @tparam T The type of option instrument instances.
 */
template <class T> class OptionChainsBuilder {
    std::string product_{};
    std::string underlying_{};
    OptionSeries<T> series_{};
    std::string cfi_{};
    double strike_{};
    std::unordered_map<std::string, OptionChain<T>> chains_{};

    OptionChain<T> &getOrCreateChain(const std::string &symbol) {
        return *chains_.try_emplace(symbol, OptionChain<T>(symbol)).first;
    }

  public:
    /**
     * Creates new option chains builder.
     */
    OptionChainsBuilder() = default;

    /**
     * Changes product for futures and options on futures (underlying asset name).
     *
     * Example: "/YG".
     *
     * @param product The product for futures and options on futures (underlying asset name).
     */
    void setProduct(const std::string &product) {
        product_ = product == String::NUL || product.empty() ? String::EMPTY : product;
    }

    /**
     * Changes primary underlying symbol for options.
     *
     * Example: "C", "/YGM9"
     *
     * @param underlying The primary underlying symbol for options.
     */
    void setUnderlying(const std::string &underlying) {
        underlying_ = underlying == String::NUL || underlying.empty() ? String::EMPTY : underlying;
    }

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @param expiration The day id of expiration.
     */
    void setExpiration(std::int32_t expiration) {
        series_.expiration_ = expiration;
    }

    /**
     * Changes day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @param lastTrade The day id of last trading day.
     */
    void setLastTrade(std::int32_t lastTrade) {
        series_.lastTrade_ = lastTrade;
    }

    /**
     * Changes market value multiplier.
     * Example: 100, 33.2.
     *
     * @param multiplier The market value multiplier.
     */
    void setMultiplier(double multiplier) {
        series_.multiplier_ = multiplier;
    }

    /**
     * Changes shares per contract for options.
     *
     * Example: 1, 100.
     *
     * @param spc The shares per contract for options.
     */
    void setSPC(double spc) {
        series_.spc_ = spc;
    }

    /**
     * Changes additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * the list shall be sorted by <UNDERLYING>.
     * ```
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @param additionalUnderlyings The additional underlyings for options, including additional cash.
     */
    void setAdditionalUnderlyings(const std::string &additionalUnderlyings) {
        series_.additionalUnderlyings_ = additionalUnderlyings == String::NUL || additionalUnderlyings.empty()
                                             ? String::EMPTY
                                             : additionalUnderlyings;
    }

    /**
     * Changes maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @param mmy The maturity month-year as provided for corresponding FIX tag (200).
     */
    void setMMY(const std::string &mmy) {
        series_.mmy_ = mmy == String::NUL || mmy.empty() ? String::EMPTY : mmy;
    }

    /**
     * Changes type of option.
     * It shall use one of following values:
     * <ul>
     * <li>STAN = Standard Options
     * <li>LEAP = Long-term Equity AnticiPation Securities
     * <li>SDO = Special Dated Options
     * <li>BINY = Binary Options
     * <li>FLEX = FLexible EXchange Options
     * <li>VSO = Variable Start Options
     * <li>RNGE = Range
     * </ul>
     *
     * @param optionType The type of option.
     */
    void setOptionType(const std::string &optionType) {
        series_.optionType_ = optionType == String::NUL || optionType.empty() ? String::EMPTY : optionType;
    }

    /**
     * Changes the expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @param expirationStyle The expiration cycle style.
     */
    void setExpirationStyle(const std::string &expirationStyle) {
        series_.expirationStyle_ =
            expirationStyle == String::NUL || expirationStyle.empty() ? String::EMPTY : expirationStyle;
    }

    /**
     * Changes settlement price determination style, such as "Open", "Close".
     *
     * @param settlementStyle The settlement price determination style.
     */
    void setSettlementStyle(const std::string &settlementStyle) {
        series_.settlementStyle_ =
            settlementStyle == String::NUL || settlementStyle.empty() ? String::EMPTY : settlementStyle;
    }

    /**
     * Changes Classification of Financial Instruments code.
     * It is a mandatory field as it is the only way to distinguish Call/Put option type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     *
     * Example: "OC" for generic call, "OP" for generic put.
     *
     * @param cfi CFI code.
     */
    void setCFI(const std::string &cfi) {
        cfi_ = cfi == String::NUL || cfi.empty() ? String::EMPTY : cfi;
        series_.cfi_ = cfi_.size() < 2 ? cfi_ : cfi_[0] + std::string("X") + cfi_.substr(2);
    }

    /**
     * Changes strike price for options.
     *
     * Example: 80, 22.5.
     *
     * @param strike The strike price for options.
     */
    void setStrike(double strike) {
        strike_ = strike;
    }

    /**
     * Adds an option instrument to this builder.
     *
     * Option is added to chains for the currently set @ref ::setProduct() "product" and/or
     * @ref ::setUnderlying() "underlying" to the @ref OptionSeries "series" that corresponding
     * to all other currently set attributes. This method is safe in the sense that is ignores
     * illegal state of the builder. It only adds an option when all the following conditions are met:
     * <ul>
     *  <li>@ref ::setCFI() "CFI code" is set and starts with either "OC" for call or "OP" for put;
     *  <li>@ref ::setExpiration() "expiration" is set and is not zero;
     *  <li>@ref ::setStrike() "strike" is set and is not NaN nor Inf;
     *  <li>@ref ::setProduct() "product" or @ref ::setUnderlying() "underlying symbol" are set;
     * </ul>
     * All the attributes remain set as before after the call to this method, but
     * @ref ::getChains() "chains" are updated correspondingly.
     *
     * @param option The option to add.
     */
    void addOption(const T &option) {
        bool isCall = cfi_.starts_with("OC");

        if (!isCall && !cfi_.starts_with("OP") /*is not put*/) {
            return;
        }

        if (series_.expiration_ == 0) {
            return;
        }

        if (std::isnan(strike_) || std::isinf(strike_)) {
            return;
        }

        if (!product_.empty()) {
            getOrCreateChain(product_).addOption(series_, isCall, strike_, option);
        }

        if (!underlying_.empty()) {
            getOrCreateChain(underlying_).addOption(series_, isCall, strike_, option);
        }
    }

    /**
     * Adds an option instrument to this builder.
     *
     * Option is added to chains for the currently set @ref ::setProduct() "product" and/or
     * @ref ::setUnderlying() "underlying" to the @ref OptionSeries "series" that corresponding
     * to all other currently set attributes. This method is safe in the sense that is ignores
     * illegal state of the builder. It only adds an option when all the following conditions are met:
     * <ul>
     *  <li>@ref ::setCFI() "CFI code" is set and starts with either "OC" for call or "OP" for put;
     *  <li>@ref ::setExpiration() "expiration" is set and is not zero;
     *  <li>@ref ::setStrike() "strike" is set and is not NaN nor Inf;
     *  <li>@ref ::setProduct() "product" or @ref ::setUnderlying() "underlying symbol" are set;
     * </ul>
     * All the attributes remain set as before after the call to this method, but
     * @ref ::getChains() "chains" are updated correspondingly.
     *
     * @param option The option to add.
     */
    void addOption(std::shared_ptr<T> option) {
        addOption(*option);
    }

    /**
     * Returns a view of chains created by this builder.
     * It updates as new options are added with @ref ::addOption() "addOption" method.
     *
     * @return The view of chains created by this builder.
     */
    const std::unordered_map<std::string, OptionChain<T>> &getChains() {
        return chains_;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()