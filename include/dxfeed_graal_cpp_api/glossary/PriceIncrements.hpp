// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Common.hpp"
#include "../internal/Conf.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include <dxfeed_graal_cpp_api/entity/SharedEntity.hpp>

DXFCPP_BEGIN_NAMESPACE

/**
 * Specifies a <i>rounding behavior</i> for numerical operations capable of discarding precision. Each rounding mode
 * indicates how the least significant returned digit of a rounded result is to be calculated. If fewer digits are
 * returned than the digits needed to represent the exact numerical result, the discarded digits will be referred to
 * as the <i>discarded fraction</i> regardless of the digits' contribution to the value of the number. In other words,
 * considered as a numerical value, the discarded fraction could have an absolute value greater than one.
 *
 * https://docs.oracle.com/en/java/javase/22/docs/api/java.base/java/math/RoundingMode.html
 */
enum class RoundingMode : std::int32_t {
    /**
     * Rounding mode to round away from zero. Always increments the digit prior to a non-zero discarded fraction.
     * Note that this rounding mode never decreases the magnitude of the calculated value.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode UP Examples</b></caption>
     *<tr valign=top><th>Input Number</th><th>Input rounded to one digit<br> with `ROUNDING_MODE_UP` rounding</th>
     *<tr align=right><td>5.5</td>  <td>6</td>
     *<tr align=right><td>2.5</td>  <td>3</td>
     *<tr align=right><td>1.6</td>  <td>2</td>
     *<tr align=right><td>1.1</td>  <td>2</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-2</td>
     *<tr align=right><td>-1.6</td> <td>-2</td>
     *<tr align=right><td>-2.5</td> <td>-3</td>
     *<tr align=right><td>-5.5</td> <td>-6</td>
     *</table>
     */
    ROUNDING_MODE_UP = 0,

    /**
     * Rounding mode to round towards zero. Never increments the digit prior to a discarded fraction (i.e., truncates).
     * Note that this rounding mode never increases the magnitude of the calculated value.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode DOWN Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_DOWN` rounding</th>
     *<tr align=right><td>5.5</td>  <td>5</td>
     *<tr align=right><td>2.5</td>  <td>2</td>
     *<tr align=right><td>1.6</td>  <td>1</td>
     *<tr align=right><td>1.1</td>  <td>1</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-1</td>
     *<tr align=right><td>-1.6</td> <td>-1</td>
     *<tr align=right><td>-2.5</td> <td>-2</td>
     *<tr align=right><td>-5.5</td> <td>-5</td>
     *</table>
     */
    ROUNDING_MODE_DOWN,

    /**
     * Rounding mode to round towards positive infinity. If the result is positive, behaves as for
     * #ROUNDING_MODE_UP; if negative, behaves as for #ROUNDING_MODE_DOWN. Note that this rounding mode never
     * decreases the calculated value.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode CEILING Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_CEILING` rounding</th>
     *<tr align=right><td>5.5</td>  <td>6</td>
     *<tr align=right><td>2.5</td>  <td>3</td>
     *<tr align=right><td>1.6</td>  <td>2</td>
     *<tr align=right><td>1.1</td>  <td>2</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-1</td>
     *<tr align=right><td>-1.6</td> <td>-1</td>
     *<tr align=right><td>-2.5</td> <td>-2</td>
     *<tr align=right><td>-5.5</td> <td>-5</td>
     *</table>
     */
    ROUNDING_MODE_CEILING,

    /**
     * Rounding mode to round towards negative infinity. If the result is positive, behave as for
     * #ROUNDING_MODE_DOWN; if negative, behave as for #ROUNDING_MODE_UP. Note that this rounding mode never
     * increases the calculated value.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode FLOOR Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_FLOOR` rounding</th>
     *<tr align=right><td>5.5</td>  <td>5</td>
     *<tr align=right><td>2.5</td>  <td>2</td>
     *<tr align=right><td>1.6</td>  <td>1</td>
     *<tr align=right><td>1.1</td>  <td>1</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-2</td>
     *<tr align=right><td>-1.6</td> <td>-2</td>
     *<tr align=right><td>-2.5</td> <td>-3</td>
     *<tr align=right><td>-5.5</td> <td>-6</td>
     *</table>
     */
    ROUNDING_MODE_FLOOR,

    /**
     * Rounding mode to round towards "nearest neighbor" unless both neighbors are equidistant, in which case round up.
     * Behaves as for #ROUNDING_MODE_UP if the discarded fraction is <= 0.5; otherwise, behaves as for
     * #ROUNDING_MODE_DOWN. Note that this is the rounding mode commonly taught at school.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode HALF_UP Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_HALF_UP` rounding</th>
     *<tr align=right><td>5.5</td>  <td>6</td>
     *<tr align=right><td>2.5</td>  <td>3</td>
     *<tr align=right><td>1.6</td>  <td>2</td>
     *<tr align=right><td>1.1</td>  <td>1</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-1</td>
     *<tr align=right><td>-1.6</td> <td>-2</td>
     *<tr align=right><td>-2.5</td> <td>-3</td>
     *<tr align=right><td>-5.5</td> <td>-6</td>
     *</table>
     */
    ROUNDING_MODE_HALF_UP,

    /**
     * Rounding mode to round towards "nearest neighbor" unless both neighbors are equidistant, in which case round
     * down. Behaves as for #ROUNDING_MODE_UP if the discarded fraction is > 0.5; otherwise, behaves as for
     * #ROUNDING_MODE_DOWN.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode HALF_DOWN Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_HALF_DOWN` rounding</th>
     *<tr align=right><td>5.5</td>  <td>5</td>
     *<tr align=right><td>2.5</td>  <td>2</td>
     *<tr align=right><td>1.6</td>  <td>2</td>
     *<tr align=right><td>1.1</td>  <td>1</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-1</td>
     *<tr align=right><td>-1.6</td> <td>-2</td>
     *<tr align=right><td>-2.5</td> <td>-2</td>
     *<tr align=right><td>-5.5</td> <td>-5</td>
     *</table>
     */
    ROUNDING_MODE_HALF_DOWN,

    /**
     * Rounding mode to round towards the "nearest neighbor" unless both neighbors are equidistant, in which case, round
     * towards the even neighbor. Behaves as for #ROUNDING_MODE_HALF_UP if the digit to the left of the
     * discarded fraction is odd; behaves as for #ROUNDING_MODE_HALF_DOWN if it's even. Note that this is the
     * rounding mode that statistically minimizes cumulative error when applied repeatedly over a sequence of
     * calculations. It is sometimes known as "Banker's rounding" and is chiefly used in the USA. This rounding mode is
     * analogous to the rounding policy used for `float` and `double` arithmetic in Java.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode HALF_EVEN Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_HALF_EVEN` rounding</th>
     *<tr align=right><td>5.5</td>  <td>6</td>
     *<tr align=right><td>2.5</td>  <td>2</td>
     *<tr align=right><td>1.6</td>  <td>2</td>
     *<tr align=right><td>1.1</td>  <td>1</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>-1</td>
     *<tr align=right><td>-1.6</td> <td>-2</td>
     *<tr align=right><td>-2.5</td> <td>-2</td>
     *<tr align=right><td>-5.5</td> <td>-6</td>
     *</table>
     */
    ROUNDING_MODE_HALF_EVEN,

    /**
     * Rounding mode to assert that the requested operation has an exact result, hence no rounding is necessary. If
     * this rounding mode is specified on an operation that yields an inexact result, an Java `ArithmeticException` is
     * thrown.
     *
     *<p>Example:
     *<table border>
     * <caption><b>Rounding mode UNNECESSARY Examples</b></caption>
     *<tr valign=top><th>Input Number</th>
     *    <th>Input rounded to one digit<br> with `ROUNDING_MODE_UNNECESSARY` rounding</th>
     *<tr align=right><td>5.5</td>  <td>throw `ArithmeticException`</td>
     *<tr align=right><td>2.5</td>  <td>throw `ArithmeticException`</td>
     *<tr align=right><td>1.6</td>  <td>throw `ArithmeticException`</td>
     *<tr align=right><td>1.1</td>  <td>throw `ArithmeticException`</td>
     *<tr align=right><td>1.0</td>  <td>1</td>
     *<tr align=right><td>-1.0</td> <td>-1</td>
     *<tr align=right><td>-1.1</td> <td>throw `ArithmeticException`</td>
     *<tr align=right><td>-1.6</td> <td>throw `ArithmeticException`</td>
     *<tr align=right><td>-2.5</td> <td>throw `ArithmeticException`</td>
     *<tr align=right><td>-5.5</td> <td>throw `ArithmeticException`</td>
     *</table>
     */
    ROUNDING_MODE_UNNECESSARY,
};

/**
 * Represents rules for valid price quantization for a given instrument on a certain exchange.
 * These rules are defined as a set of price ranges with associated price increments.
 * Each price increment defines what price values are valid for the corresponding price range-valid prices shall be
 * divisible by corresponding price increment.
 * <p>
 * All price ranges shall be mutually exclusive, and they shall cover the entire space from 0 to infinity.
 * Therefore, all ranges can be represented as a sequence of numbers where increments are interleaved with range
 * limits, with extreme limits (0 and infinity) omitted for short.
 * Negative space (from negative infinity to 0) uses symmetrical price ranges.
 * <p>
 * There is a special value PriceIncrements::EMPTY that is used to represent unknown or undefined rules.
 * This value has empty textual representation and is considered to have a sole increment with value 0.
 * <p>
 * See PriceIncrements::getText() and PriceIncrements::getPriceIncrements() for details about used formats
 * and representations.
 *
 * <p><b>NOTE:</b>This class produces precise results for decimal numbers with at most 14
 * significant digits and at most 14 digits after the decimal point.
 */
struct DXFCPP_EXPORT PriceIncrements : RequireMakeShared<PriceIncrements> {
    /// The alias to a type of shared pointer to the PriceIncrements object
    using Ptr = std::shared_ptr<PriceIncrements>;

    /// The alias to a type of unique pointer to the PriceIncrements object
    using Unique = std::unique_ptr<PriceIncrements>;

    private:
    mutable JavaObjectHandle<PriceIncrements> handle_;
    mutable std::mutex mtx_{};
    mutable bool initialized_{};
    std::function<JavaObjectHandle<PriceIncrements>()> initializer_;

    void init() const;

    public:
    /**
     * Empty price increments - it has empty text and sole increment with value 0.
     */
    static const Ptr EMPTY;

    // lazy c-tor
    PriceIncrements(LockExternalConstructionTag, std::function<JavaObjectHandle<PriceIncrements>()> initializer);

    PriceIncrements(LockExternalConstructionTag, JavaObjectHandle<PriceIncrements> &&handle);

    ~PriceIncrements() noexcept override;

    /**
     * Returns an instance of price increments for specified textual representation.
     * See PriceIncrements::getText() for format specification.
     *
     * @param text The textual representation.
     * @return The price increments instance.
     * @throws JavaException("IllegalArgumentException") if text uses wrong format or contains invalid values.
     */
    static Ptr valueOf(const StringLike &text);

    /**
     * Returns an instance of price increments for a specified single increment.
     *
     * @param increment The increment.
     * @return The price increments instance.
     * @throws JavaException("IllegalArgumentException") if increment uses invalid value
     */
    static Ptr valueOf(double increment);

    /**
     * Returns an instance of price increments for specified internal representation.
     * See PriceIncrements::getPriceIncrements() for details about internal representation.
     *
     * @param increments The increments.
     * @return The price increments instance.
     * @throws JavaException("IllegalArgumentException") if data contains invalid values
     */
    static Ptr valueOf(const std::vector<double> &increments);

    /**
     * Returns an instance of price increments for specified internal representation.
     * See PriceIncrements::getPriceIncrements() for details about internal representation.
     *
     * @param increments The increments.
     * @return The price increments instance.
     * @throws JavaException("IllegalArgumentException") if data contains invalid values
     */
    static Ptr valueOf(std::initializer_list<double> increments);

    /**
     * Returns textual representation of price increments in the format:
     * <pre>
     * TEXT ::= "" | LIST
     * LIST ::= INCREMENT | RANGE "; " LIST
     * RANGE ::= INCREMENT " " UPPER_LIMIT
     * </pre>
     * Where `INCREMENT` is a price increment in the given price range and `UPPER_LIMIT` is the upper bound of that
     * range. All ranges are listed in the ascending order of upper limits, and the last range is considered to extend
     * toward infinity and is therefore specified without upper limit. All increments and limits are finite positive
     * numbers. The case with empty text is a special stub used for PriceIncrements::EMPTY value, it uses a sole
     * increment with value 0.
     *
     * @return The textual representation.
     */
    std::string getText() const;

    /**
     * Returns internal representation of price increments as a single array of double values.
     * This array specifies all numbers from textual representation (see PriceIncrements::getText()) in the same order.
     * Therefore, numbers at even positions are increments and numbers at odd positions are upper limits.
     * The array always has an odd length - the infinite upper limit of the last range is always omitted, and
     * the first increment (for price range adjacent to 0) is always included even for PriceIncrements::EMPTY value.
     *
     * @return The internal representation of price increments.
     */
    std::vector<double> getPriceIncrements() const;

    /**
     * Returns the first price increment (for price range adjacent to 0), usually the smallest one.
     * Returns 0 for PriceIncrements::EMPTY price increments.
     *
     * @return The first price increment.
     */
    double getPriceIncrement() const;

    /**
     * Returns price increment which shall be applied to the specified price.
     * If the price is Not-a-Number (`NaN`), then the first price increment is returned.
     * If price is a breakpoint between two ranges, then a minimum of upward and downward increments is returned.
     * This method is equivalent to calling @ref PriceIncrements::getPriceIncrement(double, int) "PriceIncrements::getPriceIncrement(price, 0)".
     *
     * @param price The price.
     * @return The price increment.
     */
    double getPriceIncrement(double price) const;

    /**
     * Returns price increment which shall be applied to the specified price in the specified direction.
     * If the price is Not-a-Number (`NaN`), then the first price increment is returned.
     * If the price is a breakpoint between two ranges and a direction is 0, then a minimum of upward and downward
     * increments is returned.
     *
     * @param price The price.
     * @param direction The direction.
     * @return The price increment.
     */
    double getPriceIncrement(double price, std::int32_t direction) const;

    /**
     * Returns first price precision (for price range adjacent to 0), usually the largest one.
     * Returns 0 for PriceIncrements::EMPTY price increments.
     *
     * @return The first price precision.
     */
    std::int32_t getPricePrecision() const;

    /**
     * Returns price precision for the price range which contains a specified price.
     * Price precision is a number of decimal digits after decimal point that are needed
     * to represent all valid prices in the given price range.
     * This method returns price precision in the interval [0, 18] inclusive.
     * If the price is Not-a-Number (`NaN`), then the first price precision is returned.
     * If the price is a breakpoint between two ranges, then precision of the lower range is returned.
     *
     * @param price The price.
     * @return The price precision.
     */
    std::int32_t getPricePrecision(double price) const;

    /**
     * Returns specified price rounded to nearest valid value.
     * If the price is Not-a-Number (`NaN`), then NaN is returned.
     * If the appropriate price increment is 0, then the specified price is returned as is.
     * This method is equivalent to calling @ref PriceIncrements::roundPrice(double, int) "PriceIncrements::roundPrice(price, 0)".
     *
     * @param price The price to round.
     * @return The rounded price.
     */
    double roundPrice(double price) const;

    /**
     * Returns specified price rounded in the specified direction to the nearest value
     * that is valid, according to price increment rules.
     * If the price is Not-a-Number (`NaN`), then NaN is returned.
     * If the appropriate price increment is 0, then the specified price is returned as is.
     * If a direction is 0, then the price is rounded to the nearest valid value.
     *
     * @param price The price to round.
     * @param direction The direction.
     * @return The rounded price.
     */
    double roundPrice(double price, std::int32_t direction) const;

    /**
     * Returns specified price rounded according to specified rounding mode to nearest value
     * that is valid, according to price increment rules.
     * If the price is Not-a-Number (`NaN`), then NaN is returned.
     * If the appropriate price increment is 0, then the specified price is returned as is.
     *
     * @param price The price to round.
     * @param roundingMode The rounding mode.
     * @return The rounded price.
     */
    double roundPrice(double price, RoundingMode roundingMode) const;

    /**
     * Returns specified price incremented in the specified direction
     * by appropriate increment and then rounded to the nearest valid value.
     * If the price is Not-a-Number (`NaN`), then NaN is returned.
     * If the appropriate price increment is 0, then the specified price is returned as is.
     * This method is equivalent to calling @ref PriceIncrements::incrementPrice(double, int, double) "PriceIncrements::incrementPrice(price, direction, 0)".
     *
     * @param price The price to increment.
     * @param direction The direction.
     * @return The new price.
     * @throws JavaException("IllegalArgumentException") if the direction is 0
     */
    double incrementPrice(double price, std::int32_t direction) const;

    /**
     * Returns specified price incremented in the specified direction by the maximum of
     * a specified step and appropriate increment, and then rounded to the nearest valid value.
     * If the price is Not-a-Number (`NaN`), then NaN is returned.
     * If both step and appropriate price increment are 0, then the specified price is returned as is.
     * Note that a step must be positive even for negative directions.
     *
     * @param price The price to increment.
     * @param direction The direction.
     * @param step The step.
     * @return The new price.
     * @throws JavaException("IllegalArgumentException") if the direction is 0, or the step uses an invalid value
     */
    double incrementPrice(double price, std::int32_t direction, double step) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const PriceIncrements &other) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const PriceIncrements::Ptr &other) const {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t hashCode() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation
     */
    std::string toString() const override;

    friend std::ostream &operator<<(std::ostream &os, const PriceIncrements &p) {
        return os << p.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const PriceIncrements::Ptr &p) {
        return os << p->toString();
    }
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::PriceIncrements> {
    std::size_t operator()(const dxfcpp::PriceIncrements &p) const noexcept {
        return p.hashCode();
    }
};