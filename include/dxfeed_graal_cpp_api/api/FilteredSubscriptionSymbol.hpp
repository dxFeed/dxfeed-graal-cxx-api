// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

/**
 * This marker interface marks subscription symbol classes (like TimeSeriesSubscriptionSymbol)
 * that attach "filters" to the actual symbols. Filtered subscription symbols implement their `operator ==`
 * and `std::hash<>` based on their symbol only, without considering their "filter" part (for example, a
 * TimeSeriesSubscriptionSymbol has a @ref TimeSeriesSubscriptionSymbol::getFromTime() "fromTime" filter on a time range
 * of events).
 *
 * <p>DXFeedSubscription has the following behavior for filtered symbols. There can be only one
 * active filter per symbol. Adding new filtered symbol with the same symbol but different filter
 * removes the old one from subscription, adds the new one, and <em>notifies</em>
 * ObservableSubscriptionChangeListener <em>about this subscription change</em>.
 * The later is a special behaviour for filtered subscription symbols, because on other types of
 * symbols (like StringSymbol, for example) there is no notification when replacing one symbol with the other that
 * is "equal" to the first one.
 */
struct FilteredSubscriptionSymbol {};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()