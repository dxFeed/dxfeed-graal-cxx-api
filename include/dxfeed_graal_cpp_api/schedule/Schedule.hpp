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
    void *handle_;

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
};

} // namespace dxfcpp