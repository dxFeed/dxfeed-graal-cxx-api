// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT Day {
    friend struct Schedule;

    /// The alias to a type of shared pointer to the Day object
    using Ptr = std::shared_ptr<Day>;

    /// The alias to a type of unique pointer to the Day object
    using Unique = std::unique_ptr<Day>;

  private:
    JavaObjectHandle<Day> handle_;

    Day() noexcept;

  public:
    /**
     * Returns year, month and day numbers packed in the following way:
     * <pre>YearMonthDay = year * 10000 + month * 100 + day</pre>
     * For example, September 28, 1977 has value 19770928.
     *
     * @return Packed year, month and day numbers
     */
    std::int32_t getYearMonthDay() const noexcept;

    /**
     * Returns following day accepted by specified filter.
     * This method looks for appropriate day up to a year in the future. If no such day was found
     * within one year this method will return <b>null</b>.
     *
     * @param filter the filter to test days
     * @return nearest following day that is accepted by the filter
     */
    Day::Ptr findNextDay(const DayFilter &filter) const noexcept;
};

} // namespace dxfcpp