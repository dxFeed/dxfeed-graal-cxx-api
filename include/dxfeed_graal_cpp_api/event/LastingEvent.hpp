// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <memory>

namespace dxfcpp {

/**
 * Represents up-to-date information about some condition or state of an external entity that updates in real-time.
 * For example, a Quote is an up-to-date information about best bid and best offer for a specific symbol.
 *
 * Lasting events are conflated for each symbol. Last event for each symbol is always delivered to event listeners on
 * subscription, but intermediate (next-to-last) events are not queued anywhere, they are simply discarded as stale
 * events. More recent events represent an up-to-date information about some external entity.
 *
 * Lasting events can be used with DXFeed::getLastEvent() and DXFeed::getLastEvents methods to retrieve last events
 * for each symbol.
 *
 * Note, that subscription to all lasting events of a specific type via WildcardSymbol::ALL symbol object does not
 * benefit from the above advantages of lasting events.
 */
struct DXFCPP_EXPORT LastingEvent {
    /// The alias to a type of shared pointer to the LastingEvent object
    using Ptr = std::shared_ptr<LastingEvent>;
};

} // namespace dxfcpp