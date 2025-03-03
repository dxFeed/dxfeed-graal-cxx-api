// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

template <typename T> class OptionChainsBuilder;

/**
 * Series of call and put options with different strike sharing the same attributes of expiration, last trading day,
 * spc, multiplies, etc.
 *
 * <h3>Threads and locks</h3>
 * This class is <b>NOT</b> thread-safe and cannot be used from multiple threads without external synchronization.
 *
 * @tparam T The type of option instrument instances.
 */
template <typename T> class OptionSeries final {
    friend class OptionChainsBuilder<T>;

    std::int32_t expiration_ = 0;
    std::int32_t lastTrade_ = 0;
    double multiplier_ = 0.0;
    double spc_ = 0.0;
    std::string additionalUnderlyings_{};
    std::string mmy_{};
    std::string optionType_{};
    std::string expirationStyle_{};
    std::string settlementStyle_{};
    std::string cfi_{};

    std::map<double, std::shared_ptr<T>> calls_{};
    std::map<double, std::shared_ptr<T>> puts_{};

    mutable std::vector<double> strikes_{}; // Cached list of strikes

    public:
    /**
     * Default constructor for the OptionSeries class.
     *
     * @return A default-initialized instance of OptionSeries.
     */
    OptionSeries() = default;

    /**
     * Returns day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @return day id of expiration.
     */
    std::int32_t getExpiration() const {
        return expiration_;
    }

    /**
     * Returns day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @return The day id of last trading day.
     */
    std::int32_t getLastTrade() const {
        return lastTrade_;
    }

    /**
     * Returns market value multiplier.
     * Example: 100, 33.2.
     *
     * @return The market value multiplier.
     */
    double getMultiplier() const {
        return multiplier_;
    }

    /**
     * Returns shares per contract for options.
     * Example: 1, 100.
     *
     * @return The shares per contract for options.
     */
    double getSPC() const {
        return spc_;
    }

    /**
     * Returns additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * ```
     * The list shall be sorted by <UNDERLYING>.
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @return The additional underlyings for options, including additional cash.
     */
    const std::string &getAdditionalUnderlyings() const {
        return additionalUnderlyings_;
    }

    /**
     * Returns maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @return The maturity month-year as provided for corresponding FIX tag (200).
     */
    const std::string &getMMY() const {
        return mmy_;
    }

    /**
     * Returns type of option.
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
     * @return The type of option.
     */
    const std::string &getOptionType() const {
        return optionType_;
    }

    /**
     * Returns expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @return The expiration cycle style.
     */
    const std::string &getExpirationStyle() const {
        return expirationStyle_;
    }

    /**
     * Returns settlement price determination style, such as "Open", "Close".
     *
     * @return The settlement price determination style.
     */
    const std::string &getSettlementStyle() const {
        return settlementStyle_;
    }

    /**
     * Returns Classification of Financial Instruments code.
     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
     *
     * @return The CFI code.
     */
    const std::string &getCFI() const {
        return cfi_;
    }

    /**
     * Returns a sorted map of all calls from strike to a corresponding option instrument.
     *
     * @return A sorted map of all calls from strike to a corresponding option instrument.
     */
    const std::map<double, std::shared_ptr<T>> &getCalls() const {
        return calls_;
    }

    /**
     * Returns a sorted map of all puts from strike to a corresponding option instrument.
     *
     * @return A sorted map of all puts from strike to a corresponding option instrument.
     */
    const std::map<double, std::shared_ptr<T>> &getPuts() const {
        return puts_;
    }

    /**
     * Returns a list of all strikes in ascending order.
     *
     * @return A list of all strikes in ascending order.
     */
    const std::vector<double> &getStrikes() const {
        if (strikes_.empty()) {
            std::set<double> strikesSet{};

            for (const auto &call : calls_) {
                strikesSet.insert(call.first);
            }

            for (const auto &put : puts_) {
                strikesSet.insert(put.first);
            }

            strikes_.assign(strikesSet.begin(), strikesSet.end());
        }

        return strikes_;
    }

    /**
     * Returns n strikes which are centered around a specified strike value.
     *
     * @param n The maximal number of strikes to return.
     * @param strike The center strike.
     * @return n strikes which are centered around a specified strike value.
     */
    std::vector<double> getNStrikesAround(std::size_t n, double strike) const {
        const auto &strikesVector = getStrikes();
        const auto it = std::lower_bound(strikesVector.begin(), strikesVector.end(), strike);
        const std::size_t index = std::distance(strikesVector.begin(), it);
        const std::size_t fromIndex = (index < n / 2) ? 0 : (index - n / 2);
        const std::size_t toIndex = std::min(strikesVector.size(), fromIndex + n);

        return std::vector<double>(strikesVector.begin() + fromIndex, strikesVector.begin() + toIndex);
    }

    bool operator==(const OptionSeries &other) const {
        return expiration_ == other.expiration_ && lastTrade_ == other.lastTrade_ &&
               math::equals(multiplier_, other.multiplier_) && math::equals(spc_, other.spc_) &&
               additionalUnderlyings_ == other.additionalUnderlyings_ && expirationStyle_ == other.expirationStyle_ &&
               mmy_ == other.mmy_ && optionType_ == other.optionType_ && cfi_ == other.cfi_ &&
               settlementStyle_ == other.settlementStyle_;
    }

    bool operator!=(const OptionSeries &other) const {
        return !(*this == other);
    }

    bool operator<(const OptionSeries &other) const {
        if (expiration_ != other.expiration_) {
            return expiration_ < other.expiration_;
        }

        if (lastTrade_ != other.lastTrade_) {
            return lastTrade_ < other.lastTrade_;
        }

        if (multiplier_ != other.multiplier_) {
            return multiplier_ < other.multiplier_;
        }

        if (spc_ != other.spc_) {
            return spc_ < other.spc_;
        }

        if (additionalUnderlyings_ != other.additionalUnderlyings_) {
            return additionalUnderlyings_ < other.additionalUnderlyings_;
        }

        if (mmy_ != other.mmy_) {
            return mmy_ < other.mmy_;
        }

        if (optionType_ != other.optionType_) {
            return optionType_ < other.optionType_;
        }

        if (expirationStyle_ != other.expirationStyle_) {
            return expirationStyle_ < other.expirationStyle_;
        }

        if (settlementStyle_ != other.settlementStyle_) {
            return settlementStyle_ < other.settlementStyle_;
        }

        return cfi_ < other.cfi_;
    }

    void addOption(bool isCall, double strike, std::shared_ptr<T> option) {
        if (auto &map = isCall ? calls_ : puts_; map.emplace(strike, option).second) {
            strikes_.clear();
        }
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation.
     */
    std::string toString() const {
        std::ostringstream ss{};

        ss << "expiration=" << expiration_;

        if (lastTrade_ != 0) {
            ss << ", lastTrade=" << lastTrade_;
        }

        if (multiplier_ != 0) {
            ss << ", multiplier=" << multiplier_;
        }

        if (spc_ != 0) {
            ss << ", spc=" << spc_;
        }

        if (!additionalUnderlyings_.empty()) {
            ss << ", additionalUnderlyings=" << additionalUnderlyings_;
        }

        if (!mmy_.empty()) {
            ss << ", mmy=" << mmy_;
        }

        if (!optionType_.empty()) {
            ss << ", optionType=" << optionType_;
        }

        if (!expirationStyle_.empty()) {
            ss << ", expirationStyle=" << expirationStyle_;
        }

        if (!settlementStyle_.empty()) {
            ss << ", settlementStyle=" << settlementStyle_;
        }

        ss << ", cfi=" << cfi_;

        return ss.str();
    }

    friend std::ostream &operator<<(std::ostream &os, const OptionSeries &series) {
        os << series.toString();

        return os;
    }

    std::size_t hashCode() const {
        auto result = static_cast<std::size_t>(expiration_);

        hashCombine(result, lastTrade_);
        hashCombine(result, multiplier_);
        hashCombine(result, spc_);
        hashCombine(result, additionalUnderlyings_);
        hashCombine(result, mmy_);
        hashCombine(result, optionType_);
        hashCombine(result, expirationStyle_);
        hashCombine(result, settlementStyle_);
        hashCombine(result, cfi_);

        return result;
    }
};

DXFCPP_END_NAMESPACE

template <typename T> struct std::hash<dxfcpp::OptionSeries<T>> {
    std::size_t operator()(const dxfcpp::OptionSeries<T> &optionSeries) const noexcept {
        return optionSeries.hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()