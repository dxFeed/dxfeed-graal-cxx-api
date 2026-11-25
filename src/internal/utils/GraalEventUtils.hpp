// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "GraalArena.hpp"

#include <dxfg_v4_api.h>
#include <new>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

/**
 * Creates the SDK structure of an event and fills it: the body of `toGraal()` of the event types.
 *
 * The class is set before the fill, so that `MarketEvent::fillGraalData()` can name the event type in the error about
 * an event symbol that is not valid UTF-8. When the fill throws (a string field that is not valid
 * UTF-8), the strings created before are freed together with the structure, and the exception goes on.
 *
 * Under a GraalArena::Scope the structure is taken from the arena of the scope and is not freed on an exception: the
 * arena frees it with the strings.
 *
 * @tparam Native The SDK structure of the event (`dxfg_v4_quote_t`)
 * @param clazz The class of the event (`DXFG_EVENT_QUOTE`)
 * @param fill Fills the structure (`fillGraalData()` of the event)
 * @param freeData Frees the strings of the structure (`freeGraalData()` of the event); safe for the fields not set yet
 * @return The new structure
 * @throws InvalidArgumentException if a string field of the event is not valid UTF-8.
 */
template <typename Native, typename Fill, typename FreeData>
void *newGraalEvent(dxfg_v4_event_clazz_t clazz, Fill &&fill, FreeData &&freeData) {
    static_assert(std::is_trivially_destructible_v<Native>);

    auto *arena = GraalArena::current();
    auto *graalNative =
        arena != nullptr ? new (arena->allocate(sizeof(Native), alignof(Native))) Native{} : new Native{};

    // dxfg_v4_event_type_t is the first member of every event structure.
    static_cast<dxfg_v4_event_type_t *>(static_cast<void *>(graalNative))->clazz = clazz;

    try {
        fill(graalNative);
    } catch (...) {
        if (arena == nullptr) {
            freeData(graalNative);
            delete graalNative;
        }

        throw;
    }

    return graalNative;
}

DXFCPP_END_NAMESPACE
