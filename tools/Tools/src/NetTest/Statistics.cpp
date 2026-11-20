// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Statistics.hpp"

#include <algorithm>
#include <chrono>
#include <limits>

namespace dxfcpp::tools::nettest {

namespace {

constexpr std::int64_t NANOS_PER_SECOND = 1'000'000'000;
constexpr std::int64_t NANOS_PER_MICRO = 1'000;
constexpr std::int64_t NANOS_PER_MILLI = 1'000'000;

constexpr std::int64_t BYTES_PER_MB = 1024 * 1024;

// The shortest last block that the reporter prints at the stop.
constexpr std::int64_t MIN_LAST_BLOCK_NANOS = NANOS_PER_SECOND;

constexpr const char *ABSENT_TEXT = "n/a";

bool present(std::int64_t value) {
    return value >= 0;
}

std::string megabytes(std::int64_t bytes) {
    return present(bytes) ? groupDigits(bytes / BYTES_PER_MB) + " MB" : ABSENT_TEXT;
}

// The growth of a cumulative counter, or absent when either value is absent or the counter went back.
std::int64_t growth(std::int64_t previous, std::int64_t current) {
    return present(previous) && present(current) && current >= previous ? current - previous : RUNTIME_VALUE_ABSENT;
}

// The sum of two growths, absent when either is absent.
std::int64_t sum(std::int64_t first, std::int64_t second) {
    return present(first) && present(second) ? first + second : RUNTIME_VALUE_ABSENT;
}

} // namespace

std::string groupDigits(std::int64_t value) {
    const bool negative = value < 0;
    // The magnitude in an unsigned type, so that the lowest int64 value works as well.
    const auto magnitude = negative ? 0 - static_cast<std::uint64_t>(value) : static_cast<std::uint64_t>(value);
    const std::string digits = std::to_string(magnitude);
    std::string result;

    for (std::size_t index = 0; index < digits.size(); ++index) {
        if (index != 0 && (digits.size() - index) % 3 == 0) {
            result += ',';
        }

        result += digits[index];
    }

    return negative ? "-" + result : result;
}

std::int64_t recordsPerSecond(std::int64_t records, std::int64_t periodNanos) {
    if (periodNanos <= 0 || records <= 0) {
        return 0;
    }

    if (records <= std::numeric_limits<std::int64_t>::max() / NANOS_PER_SECOND) {
        return records * NANOS_PER_SECOND / periodNanos;
    }

    return static_cast<std::int64_t>(static_cast<long double>(records) * NANOS_PER_SECOND / periodNanos);
}

std::int64_t lagMicros(std::int64_t records, std::int64_t weightedNanos) {
    if (records <= 0 || weightedNanos <= 0) {
        return 0;
    }

    return weightedNanos / records / NANOS_PER_MICRO;
}

std::int64_t meanLagMicros(std::int64_t records, std::int64_t lagSumNanos) {
    if (records <= 0) {
        return 0;
    }

    // C++ divides toward zero, as the specification wants for a negative sum.
    return lagSumNanos / records / NANOS_PER_MICRO;
}

std::string formatStatistics(const StatisticsSnapshot &snapshot) {
    const bool producer = snapshot.side == StatisticsSide::PRODUCER;
    const auto lag = [producer](std::int64_t records, std::int64_t nanos) {
        return producer ? lagMicros(records, nanos) : meanLagMicros(records, nanos);
    };
    std::int64_t records = 0;
    std::int64_t weightedNanos = 0;

    for (const auto &delta : snapshot.deltas) {
        records += delta.records;
        weightedNanos += delta.weightedNanos;
    }

    std::string text = std::string(producer ? "{*NetTestProducerSide-" : "{*NetTestConsumerSide-") + snapshot.name +
                       "*} data " + groupDigits(recordsPerSecond(records, snapshot.periodNanos)) + " rps, lag " +
                       groupDigits(lag(records, weightedNanos)) + " us, instances " +
                       std::to_string(snapshot.deltas.size()) + ", connections " + std::to_string(snapshot.connections);

    for (std::size_t index = 0; index < snapshot.deltas.size(); ++index) {
        const auto &delta = snapshot.deltas[index];

        text += "\n    " + snapshot.name + "." + std::to_string(index + 1) + " data " +
                groupDigits(recordsPerSecond(delta.records, snapshot.periodNanos)) + " rps, lag " +
                groupDigits(lag(delta.records, delta.weightedNanos)) + " us";
    }

    return text;
}

StatisticsSnapshot makeSnapshot(const std::string &name, const StatisticsSample &previous,
                                const StatisticsSample &current, std::int64_t periodNanos, StatisticsSide side) {
    StatisticsSnapshot snapshot;

    snapshot.side = side;
    snapshot.name = name;
    snapshot.periodNanos = periodNanos;
    snapshot.connections = current.connections;

    for (std::size_t index = 0; index < current.instances.size(); ++index) {
        InstanceTotals delta = current.instances[index];

        if (index < previous.instances.size()) {
            delta.records -= previous.instances[index].records;
            delta.weightedNanos -= previous.instances[index].weightedNanos;
        }

        snapshot.deltas.push_back(delta);
    }

    return snapshot;
}

std::string formatRuntime(const std::string &name, const RuntimeSample &previous, const RuntimeSample &current,
                          std::int64_t periodNanos) {
    const auto gcCount =
        sum(growth(previous.youngGcCount, current.youngGcCount), growth(previous.fullGcCount, current.fullGcCount));
    const auto gcTimeMs =
        sum(growth(previous.youngGcTimeMs, current.youngGcTimeMs), growth(previous.fullGcTimeMs, current.fullGcTimeMs));
    const auto cpuNanos = growth(previous.processCpuTimeNs, current.processCpuTimeNs);

    std::string text = "{*NetTestRuntime-" + name + "*} heap " + megabytes(current.heapUsedBytes) + " of " +
                       megabytes(current.heapCommittedBytes) + ", gc ";

    text += present(gcCount) && present(gcTimeMs) ? groupDigits(gcCount) + " (" + groupDigits(gcTimeMs) + " ms)"
                                                  : std::string(ABSENT_TEXT);
    text += ", cpu ";
    // The CPU nanoseconds of the period are far below the int64 range divided by 100.
    text += present(cpuNanos) && periodNanos > 0 ? groupDigits(cpuNanos * 100 / periodNanos) + "%"
                                                 : std::string(ABSENT_TEXT);

    return text;
}

ReporterClock systemReporterClock(StopControl &stop) {
    using Clock = std::chrono::system_clock;

    ReporterClock clock;

    clock.nowNanos = [] {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
    };
    clock.sleepUntilNanos = [&stop](std::int64_t nanos) {
        const Clock::time_point time{std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(nanos))};

        return !stop.waitUntil(time);
    };

    return clock;
}

void runReporter(const std::string &name, std::int64_t periodMillis, const ReporterClock &clock,
                 const StatisticsSource &source, const StatisticsSink &sink, StatisticsSide side) {
    const std::int64_t periodNanos = std::clamp<std::int64_t>(periodMillis, 1, MAX_WAIT_MILLIS) * NANOS_PER_MILLI;
    auto previous = source();
    auto previousTime = clock.nowNanos();
    bool primed = false;

    // The block and, after it, the runtime line, in one text, so that nothing printed elsewhere comes between them.
    const auto print = [&](const StatisticsSample &current, std::int64_t length) {
        std::string text = formatStatistics(makeSnapshot(name, previous, current, length, side));

        if (previous.runtime.has_value() && current.runtime.has_value()) {
            text += "\n" + formatRuntime(name, *previous.runtime, *current.runtime, length);
        }

        sink(text + "\n");
    };

    // The ticks are the multiples of the period of the wall clock; the next one is always after the current time, so a
    // late wake-up never moves the later ticks.
    while (clock.sleepUntilNanos((clock.nowNanos() / periodNanos + 1) * periodNanos)) {
        const auto current = source();
        const auto currentTime = clock.nowNanos();

        if (primed) {
            print(current, currentTime - previousTime);
        }

        primed = true;
        previous = current;
        previousTime = currentTime;
    }

    const auto current = source();
    const auto currentTime = clock.nowNanos();

    // A shorter last block is mostly the noise of its few batches, so it is not printed.
    if (currentTime - previousTime >= MIN_LAST_BLOCK_NANOS) {
        print(current, currentTime - previousTime);
    }
}

} // namespace dxfcpp::tools::nettest
