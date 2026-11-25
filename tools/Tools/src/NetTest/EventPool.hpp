// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The event pool of one NetTest instance: one pre-built event per symbol, a table of prices and sizes, and the batch
// fill that stamps the time, the sequence, the price and the size.

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "Args.hpp"
#include "SymbolPool.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace dxfcpp::tools::nettest {

/// The events of one publishing call (Trade or TimeAndSale, by the kind of the pool).
using EventBatch = std::vector<std::shared_ptr<EventType>>;

/// Packs the time and the sequence as Trade::getTimeSequence() and TimeAndSale::getIndex() do: the seconds in the
/// higher 32 bits, the milliseconds in bits 22 to 31, the sequence (22 bits, the rest is dropped) in bits 0 to 21.
std::int64_t packTimeSequence(std::int64_t epochNanos, std::uint32_t sequence);

/// Not thread-safe: one pool for each generator thread.
class EventPool {
    EventKind kind_;
    bool reuse_;
    Symbols symbols_;
    std::uint64_t seed_;
    std::uint64_t initialState_;
    std::uint64_t random_;
    std::uint32_t sequence_ = 0;
    std::vector<double> prices_{};
    std::vector<double> sizes_{};
    std::vector<std::shared_ptr<EventType>> events_{};

    public:
    static constexpr std::size_t TABLE_SIZE = 1024;
    static constexpr std::uint32_t SEQUENCE_MASK = (1U << 22) - 1;

    /// The seed of the instance (from 1): 0x9E3779B97F4A7C15 * instance, wrapping.
    static std::uint64_t seedOf(std::int32_t instance);

    /// The state of the generator for the seed.
    static std::uint64_t initialStateOf(std::uint64_t seed);

    /// One draw of xorshift64*; advances the state.
    static std::uint64_t nextDraw(std::uint64_t &state);

    /// Builds one event per symbol of the whole set (the symbols are not sliced between publishers) and the table of
    /// the prices and sizes from the draws of the instance. With `reuse` a batch holds the pool events, which are
    /// stamped in place; otherwise it holds a copy per event.
    /// @param instance the number of the instance, from 1.
    EventPool(std::int32_t instance, Symbols symbols, EventKind kind, bool reuse);

    std::uint64_t seed() const {
        return seed_;
    }

    std::uint64_t initialState() const {
        return initialState_;
    }

    /// The number of the pool events, the number of the symbols.
    std::size_t size() const {
        return events_.size();
    }

    /// The pool event of the symbol `index`.
    const std::shared_ptr<EventType> &poolEvent(std::size_t index) const {
        return events_[index];
    }

    /// The sequence that the next event takes (before the 22-bit mask).
    std::uint32_t nextSequence() const {
        return sequence_;
    }

    /// The prices and the sizes of the table (TABLE_SIZE rows each).
    const std::vector<double> &prices() const {
        return prices_;
    }

    const std::vector<double> &sizes() const {
        return sizes_;
    }

    /// Fills the batch (clearing it first) with `count` events of random symbols at the time given in nanoseconds
    /// since the Unix epoch: each event gets the time, the nano part, the next sequence of the instance, and a price
    /// and a size from the table. With `reuse` a symbol drawn twice appears twice, as one object with the later stamp.
    void fillAt(EventBatch &batch, std::size_t count, std::int64_t epochNanos);

    /// The same at the current time of the system clock.
    void fill(EventBatch &batch, std::size_t count);
};

} // namespace dxfcpp::tools::nettest
