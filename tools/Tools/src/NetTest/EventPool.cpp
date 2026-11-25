// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "EventPool.hpp"

#include <chrono>
#include <utility>

namespace dxfcpp::tools::nettest {

namespace {

constexpr std::int64_t NANOS_PER_SECOND = 1'000'000'000;
constexpr std::int64_t NANOS_PER_MILLI = 1'000'000;
constexpr std::uint64_t SEED_STEP = 0x9E3779B97F4A7C15ULL;
constexpr std::uint64_t STATE_ADDEND = 0x2545F4914F6CDD1DULL;
constexpr std::uint64_t DRAW_MULTIPLIER = 0x2545F4914F6CDD1DULL;

// The time of the system clock in nanoseconds since the Unix epoch.
std::int64_t nowEpochNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// The time and the sequence go through the packed value, not through setTime() and setSequence(), so that the packing
// is the one of the kit.
void stamp(Trade &trade, std::int64_t packed, std::int32_t nanoPart, double price, double size) {
    trade.setTimeSequence(packed);
    trade.setTimeNanoPart(nanoPart);
    trade.setPrice(price);
    trade.setSize(size);
}

void stamp(TimeAndSale &sale, std::int64_t packed, std::int32_t nanoPart, double price, double size) {
    sale.setIndex(packed);
    sale.setTimeNanoPart(nanoPart);
    sale.setPrice(price);
    sale.setSize(size);
}

} // namespace

std::int64_t packTimeSequence(std::int64_t epochNanos, std::uint32_t sequence) {
    const auto seconds = static_cast<std::uint64_t>(epochNanos / NANOS_PER_SECOND);
    const auto millis = static_cast<std::uint64_t>((epochNanos / NANOS_PER_MILLI) % 1000);

    return static_cast<std::int64_t>((seconds << 32) | (millis << 22) |
                                     static_cast<std::uint64_t>(sequence & EventPool::SEQUENCE_MASK));
}

std::uint64_t EventPool::seedOf(std::int32_t instance) {
    return SEED_STEP * static_cast<std::uint64_t>(instance);
}

std::uint64_t EventPool::initialStateOf(std::uint64_t seed) {
    const auto state = seed * SEED_STEP + STATE_ADDEND;

    return state == 0 ? STATE_ADDEND : state;
}

// xorshift64*: cheap and good enough to pick symbols and table rows.
std::uint64_t EventPool::nextDraw(std::uint64_t &state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;

    return state * DRAW_MULTIPLIER;
}

EventPool::EventPool(std::int32_t instance, Symbols symbols, EventKind kind, bool reuse)
    : kind_(kind), reuse_(reuse), symbols_(std::move(symbols)), seed_(seedOf(instance)),
      initialState_(initialStateOf(seed_)), random_(initialState_) {
    // Prices of 1.00-1000.99 and integer sizes of 1-1000: the first draw of a row is the price, the second the size.
    prices_.resize(TABLE_SIZE);
    sizes_.resize(TABLE_SIZE);

    for (std::size_t row = 0; row < TABLE_SIZE; ++row) {
        prices_[row] = static_cast<double>(100 + nextDraw(random_) % 100'000) / 100.0;
        sizes_[row] = static_cast<double>(1 + nextDraw(random_) % 1000);
    }

    events_.reserve(symbols_->size());

    for (const auto &symbol : *symbols_) {
        if (kind_ == EventKind::TRADE) {
            events_.push_back(std::make_shared<Trade>(symbol));
        } else {
            events_.push_back(std::make_shared<TimeAndSale>(symbol));
        }
    }
}

void EventPool::fillAt(EventBatch &batch, std::size_t count, std::int64_t epochNanos) {
    batch.clear();

    if (events_.empty()) {
        return;
    }

    batch.reserve(count);

    const auto nanoPart = static_cast<std::int32_t>(epochNanos % NANOS_PER_MILLI);
    const auto poolSize = static_cast<std::uint64_t>(events_.size());

    for (std::size_t item = 0; item < count; ++item) {
        const auto draw = nextDraw(random_);
        const auto &source = events_[static_cast<std::size_t>((draw >> 32) % poolSize)];
        const auto row = static_cast<std::size_t>((draw >> 8) % TABLE_SIZE);
        const auto packed = packTimeSequence(epochNanos, sequence_++);

        if (kind_ == EventKind::TRADE) {
            auto event = reuse_ ? std::static_pointer_cast<Trade>(source)
                                : std::make_shared<Trade>(*std::static_pointer_cast<Trade>(source));

            stamp(*event, packed, nanoPart, prices_[row], sizes_[row]);
            batch.push_back(std::move(event));
        } else {
            auto event = reuse_ ? std::static_pointer_cast<TimeAndSale>(source)
                                : std::make_shared<TimeAndSale>(*std::static_pointer_cast<TimeAndSale>(source));

            stamp(*event, packed, nanoPart, prices_[row], sizes_[row]);
            batch.push_back(std::move(event));
        }
    }
}

void EventPool::fill(EventBatch &batch, std::size_t count) {
    fillAt(batch, count, nowEpochNanos());
}

} // namespace dxfcpp::tools::nettest
