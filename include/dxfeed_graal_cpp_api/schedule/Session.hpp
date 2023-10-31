// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/**
 * <b>Session</b> represents a continuous period of time during which apply same rules regarding trading activity.
 * For example, <code>regular trading session</code> is a period of time consisting of one day of business activities
 * in a financial market, from the opening bell to the closing bell, when regular trading occurs.
 * <p>
 * Sessions can be either <b>trading</b> or <b>non-trading</b>, with different sets of rules and reasons to exist.
 * Sessions do not overlap with each other - rather they form consecutive chain of adjacent periods of time that
 * cover entire time scale. The point on a border line is considered to belong to following session that starts there.
 * Each session completely fits inside a certain day. Day may contain sessions with zero duration - e.g. indices
 * that post value once a day. Such sessions can be of any appropriate type, trading or non-trading.
 */
struct DXFCPP_EXPORT Session {
    friend struct Schedule;

    /// The alias to a type of shared pointer to the Session object
    using Ptr = std::shared_ptr<Session>;

    /// The alias to a type of unique pointer to the Session object
    using Unique = std::unique_ptr<Session>;

  private:
    JavaObjectHandle<Session> handle_;

    Session() noexcept;

  public:
};

}