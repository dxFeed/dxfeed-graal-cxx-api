// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/utils/StringUtils.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * Type of the candle aggregation period constitutes CandlePeriod type together its actual
 * @ref CandlePeriod::getValue() "value".
 */
struct DXFCPP_EXPORT CandleType {
    /**
     * Certain number of ticks.
     */
    static const CandleType TICK;

    /**
     * Certain number of seconds.
     */
    static const CandleType SECOND;

    /**
     * Certain number of minutes.
     */
    static const CandleType MINUTE;

    /**
     * Certain number of hours.
     */
    static const CandleType HOUR;

    /**
     * Certain number of days.
     */
    static const CandleType DAY;

    /**
     * Certain number of weeks.
     */
    static const CandleType WEEK;

    /**
     * Certain number of months.
     */
    static const CandleType MONTH;

    /**
     * Certain number of option expirations.
     */
    static const CandleType OPTEXP;

    /**
     * Certain number of years.
     */
    static const CandleType YEAR;

    /**
     * Certain volume of trades.
     */
    static const CandleType VOLUME;

    /**
     * Certain price change, calculated according to the following rules:
     * <ol>
     *     <li>high(n) - low(n) = price range</li>
     *     <li>close(n) = high(n) or close(n) = low(n)</li>
     *     <li>open(n+1) = close(n)</li>
     * </ol>
     * where n is the number of the bar.
     */
    static const CandleType PRICE;

    /**
     * Certain price change, calculated according to the following rules:
     * <ol>
     *     <li>high(n) - low(n) = price range</li>
     *     <li>close(n) = high(n) or close(n) = low(n)</li>
     *     <li>open(n+1) = close(n) + tick size, if close(n) = high(n)</li>
     *     <li>open(n+1) = close(n) - tick size, if close(n) = low(n)</li>
     * </ol>
     * where n is the number of the bar.
     */
    static const CandleType PRICE_MOMENTUM;

    /**
     * Certain price change, calculated according to the following rules:
     * <ol>
     *     <li>high(n+1) - high(n) = price range or low(n) - low(n+1) = price range</li>
     *     <li>close(n) = high(n) or close(n) = low(n)</li>
     *     <li>open(n+1) = high(n), if high(n+1) - high(n) = price range</li>
     *     <li>open(n+1) = low(n), if low(n) - low(n+1) = price range</li>
     * </ol>
     * where n is the number of the bar.
     */
    static const CandleType PRICE_RENKO;

    static const std::unordered_map<std::string, std::reference_wrapper<const CandleType>, StringHash,
                                    std::equal_to<>>
        BY_STRING;
    static const std::vector<std::reference_wrapper<const CandleType>> VALUES;

    private:
    std::string name_{};
    std::string string_{};
    std::int64_t periodIntervalMillis_{};

    CandleType(const StringLike &name, const StringLike &string,
               std::int64_t periodIntervalMillis) noexcept;

    public:
    CandleType() noexcept = default;
    virtual ~CandleType() noexcept = default;

    /**
     * Returns a candle type period in milliseconds as closely as possible.
     * Certain types like CandleType::SECOND and CandleType::DAY span a specific number of milliseconds.
     * CandleType::MONTH, CandleType::OPTEXP and CandleType::YEAR are approximate. Candle type period of
     * CandleType::TICK, CandleType::VOLUME, CandleType::PRICE, CandleType::PRICE_MOMENTUM and CandleType::PRICE_RENKO
     * is not defined and this method returns `0`.
     *
     * @return aggregation period in milliseconds.
     */
    std::int64_t getPeriodIntervalMillis() const noexcept;

    /**
     * Returns a name of this candle type
     *
     * @return A name of this candle type
     */
    const std::string &getName() const & noexcept;

    /**
     * Returns string representation of this candle type.
     * The string representation of a candle type is the shortest unique prefix of the lower case string that
     * corresponds to its @ref CandleType::getName() "name". For example, CandleType::TICK is represented as `"t"`,
     * while CandleType::MONTH is represented as `"mo"` to distinguish it from CandleType::MINUTE that is represented as
     * `"m"`.
     *
     * @return string representation of this candle price type.
     * @throws InvalidArgumentException if the string representation is invalid.
     */
    const std::string &toString() const & noexcept;

    /**
     * Parses string representation of a candle type into an object.
     * Any string that is a prefix of candle type CandleType::getName() can be parsed
     * (including the one that was returned by CandleType::toString()) and a case is ignored for parsing.
     *
     * @param s The string representation of a candle type.
     * @return A candle type.
     * @throws InvalidArgumentException if the argument is empty or invalid
     */
    static std::reference_wrapper<const CandleType> parse(const StringLike &s);

    bool operator==(const CandleType &candleType) const noexcept;
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CandleType> {
    std::size_t operator()(const dxfcpp::CandleType &candleType) const noexcept {
        return std::hash<std::string>{}(candleType.toString());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()