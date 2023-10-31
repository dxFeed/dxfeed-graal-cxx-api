// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/**
 * <b>Schedule</b> class provides API to retrieve and explore trading schedules of different exchanges
 * and different classes of financial instruments. Each instance of schedule covers separate trading schedule
 * of some class of instruments, i.e. NYSE stock trading schedule or CME corn futures trading schedule.
 * Each schedule splits entire time scale into separate @ref Day "days" that are aligned to the specific
 * trading hours of covered trading schedule.
 */
struct DXFCPP_EXPORT Schedule {
    /// The alias to a type of shared pointer to the Schedule object
    using Ptr = std::shared_ptr<Schedule>;

    /// The alias to a type of unique pointer to the Schedule object
    using Unique = std::unique_ptr<Schedule>;

  private:
    JavaObjectHandle<Schedule> handle_;

    Schedule() noexcept;

  public:
    /**
     * Returns default schedule instance for specified instrument profile.
     *
     * @param profile The instrument profile those schedule is requested
     * @return The default schedule instance for specified instrument profile
     */
    static Schedule::Ptr getInstance(std::shared_ptr<InstrumentProfile> profile) noexcept;

    /**
     * Returns default schedule instance for specified schedule definition.
     *
     * @param scheduleDefinition The schedule definition of requested schedule
     * @return The default schedule instance for specified schedule definition
     */
    static Schedule::Ptr getInstance(const std::string &scheduleDefinition) noexcept;

    /**
     * Returns schedule instance for specified instrument profile and trading venue.
     *
     * @param profile The instrument profile those schedule is requested
     * @param venue The trading venue those schedule is requested
     * @return The schedule instance for specified instrument profile and trading venue
     */
    static Schedule::Ptr getInstance(std::shared_ptr<InstrumentProfile> profile, const std::string &venue) noexcept;

    /**
     * Returns trading venues for specified instrument profile.
     *
     * @param profile The instrument profile those trading venues are requested
     * @return trading venues for specified instrument profile
     */
    static std::vector<std::string> getTradingVenues(std::shared_ptr<InstrumentProfile> profile) noexcept;

    /**
     * Sets shared defaults that are used by individual schedule instances.
     *
     * @param data The content of default data
     * @return `true` if OK
     */
    static bool setDefaults(const std::vector<char> &data) noexcept;

    /**
     * Returns session that contains specified time.
     * This method will return `Session::Ptr{nullptr}` if specified time
     * falls outside of valid date range from 0001-01-02 to 9999-12-30.
     *
     * @param time the time to search for
     * @return session that contains specified time or `Session::Ptr{nullptr}` if specified time
     * falls outside of valid date range from 0001-01-02 to 9999-12-30.
     */
    Session::Ptr getSessionByTime(std::int64_t time) const noexcept;

    /**
     * Returns day that contains specified time.
     * This method will return `Day::Ptr{nullptr}` (std::shared_ptr<Day>{nullptr}) if specified time
     * falls outside of valid date range from 0001-01-02 to 9999-12-30.
     *
     * @param time The time to search for
     * @return The day that contains specified time or `Day::Ptr{nullptr}` (std::shared_ptr<Day>{nullptr}) if
     * specified time falls outside of valid date range
     */
    Day::Ptr getDayByTime(std::int64_t time) const noexcept;
};

} // namespace dxfcpp