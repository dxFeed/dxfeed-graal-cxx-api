// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Histogram.hpp"
#include "Run.hpp"
#include "RunSupport.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dxfcpp::tools::nettest {

namespace {

// What the consumer instances share with their listeners, which run on the threads of the SDK: the live counters, the
// e2e lags of the run and of the current interval, the symbols seen, the start of the warm-up, the time of the last
// record for the watchdog and the check of the nanosecond part over the first events of all instances. The listeners
// hold it, so that a late call of a listener never outlives it.
class ConsumerState {
    std::mutex lagsMutex_{};
    LagTotals lags_{};
    // The lags of the current interval and their histogram; the reporter takes them at the end of the interval.
    LagTotals intervalLags_{};
    std::unique_ptr<Histogram> intervalHistogram_ = std::make_unique<Histogram>();
    // The histogram of the interval taken last; only the reporter touches it.
    std::unique_ptr<Histogram> taken_ = std::make_unique<Histogram>();
    std::mutex nanoMutex_{};
    std::int64_t nanoChecked_ = 0;
    bool nanoSeen_ = false;
    std::atomic<bool> nanoPending_;
    std::atomic<bool> nanoMissing_{false};
    // The steady-clock time of the last listener call with records, 0 before the first one.
    std::atomic<std::int64_t> lastRecordNanos_{0};
    // The wall-clock time of the first listener call with records, 0 before it: the start of the warm-up.
    std::atomic<std::int64_t> firstRecordEpochNanos_{0};
    // The live counters, which /metrics may share.
    std::shared_ptr<LiveCounters> liveOwner_;

    public:
    LiveCounters &live;
    // The distinct symbols received by all instances.
    std::atomic<std::int64_t> symbolsSeen{0};

    ConsumerState(std::shared_ptr<LiveCounters> counters, bool nanoCheck)
        : nanoPending_(nanoCheck), liveOwner_(std::move(counters)), live(*liveOwner_) {
    }

    bool nanoPending() const {
        return nanoPending_.load(std::memory_order_relaxed);
    }

    bool nanoMissing() const {
        return nanoMissing_.load();
    }

    std::int64_t lastRecordNanos() const {
        return lastRecordNanos_.load(std::memory_order_relaxed);
    }

    std::int64_t firstRecordEpochNanos() const {
        return firstRecordEpochNanos_.load(std::memory_order_relaxed);
    }

    LagTotals lags() {
        std::lock_guard<std::mutex> lock(lagsMutex_);

        return lags_;
    }

    // Counts the records of one listener call of instance `index` (from 0) at the wall-clock time `epochNanos`: their
    // totals, each of their lags and the buckets of /metrics.
    void addRecords(std::size_t index, std::int64_t epochNanos, const LagTotals &lags,
                    const std::vector<std::int64_t> &values) {
        auto &counters = live.instances[index];
        std::int64_t unset = 0;
        LagBuckets buckets{};
        std::int64_t positiveSum = 0;

        // The buckets and the sum of /metrics take a negative lag as 0.
        for (const auto value : values) {
            ++buckets[lagBucketIndex(value)];
            positiveSum += value > 0 ? value : 0;
        }

        firstRecordEpochNanos_.compare_exchange_strong(unset, epochNanos, std::memory_order_relaxed);
        counters.records.fetch_add(lags.count, std::memory_order_relaxed);
        counters.weightedNanos.fetch_add(lags.sum, std::memory_order_relaxed);
        live.addLags(buckets, lags.count, positiveSum, lags.negative);

        {
            std::lock_guard<std::mutex> lock(lagsMutex_);

            lags_.add(lags);
            intervalLags_.add(lags);

            // The histogram takes a negative lag as 0.
            for (const auto value : values) {
                intervalHistogram_->record(value);
            }
        }

        lastRecordNanos_.store(steadyNanos(), std::memory_order_relaxed);
    }

    // Counts the nanosecond parts of the events of one listener call, in their order, up to NANO_CHECK_EVENTS events in
    // all; when they are all 0, marks the nanosecond part missing, which stops the run.
    void checkNanoParts(const std::vector<std::int32_t> &nanoParts) {
        std::lock_guard<std::mutex> lock(nanoMutex_);

        if (!nanoPending_.load()) {
            return;
        }

        for (const auto nanoPart : nanoParts) {
            if (nanoChecked_ >= NANO_CHECK_EVENTS) {
                break;
            }

            ++nanoChecked_;
            nanoSeen_ = nanoSeen_ || nanoPart != 0;
        }

        if (nanoSeen_) {
            nanoPending_.store(false);
        } else if (nanoChecked_ >= NANO_CHECK_EVENTS) {
            nanoPending_.store(false);
            nanoMissing_.store(true);
        }
    }

    // Takes the lags of the current interval and starts the next one; the histogram stays valid until the next call.
    // Called by the reporter only.
    std::pair<LagTotals, const Histogram *> takeInterval() {
        taken_->reset();

        std::lock_guard<std::mutex> lock(lagsMutex_);
        const auto lags = intervalLags_;

        std::swap(intervalHistogram_, taken_);
        intervalLags_ = {};

        return {lags, taken_.get()};
    }
};

// The listener of one consumer instance: counts the records, sums their e2e lags and marks the symbols of its slice; it
// runs on the threads of the SDK.
class ConsumerListener {
    std::shared_ptr<ConsumerState> state_;
    std::size_t index_;
    EventKind kind_;
    // The slice of the symbols and which of them came.
    std::unordered_map<std::string, std::size_t> symbolIndex_{};
    std::mutex seenMutex_{};
    std::vector<bool> seen_{};
    std::size_t seenCount_ = 0;
    std::atomic<bool> allSeen_;

    // Marks a received symbol; the caller holds seenMutex_.
    void markSeen(const std::string &symbol) {
        const auto found = symbolIndex_.find(symbol);

        if (found == symbolIndex_.end() || seen_[found->second]) {
            return;
        }

        seen_[found->second] = true;
        ++seenCount_;
        state_->symbolsSeen.fetch_add(1, std::memory_order_relaxed);

        if (seenCount_ == seen_.size()) {
            allSeen_.store(true, std::memory_order_relaxed);
        }
    }

    public:
    ConsumerListener(std::shared_ptr<ConsumerState> state, std::size_t index, EventKind kind,
                     const std::vector<std::string> &slice)
        : state_(std::move(state)), index_(index), kind_(kind), seen_(slice.size(), false), allSeen_(slice.empty()) {
        for (std::size_t symbol = 0; symbol < slice.size(); ++symbol) {
            symbolIndex_.emplace(slice[symbol], symbol);
        }
    }

    void receive(const std::vector<std::shared_ptr<EventType>> &events) {
        // One clock read per call of the listener.
        const auto now = systemNanos();
        const bool nanoPending = state_->nanoPending();
        std::vector<std::int32_t> nanoParts{};
        LagTotals lags{};
        std::vector<std::int64_t> values{};

        values.reserve(events.size());
        // Once every symbol of the slice came, the symbols are not looked up anymore.
        std::unique_lock<std::mutex> seenLock(seenMutex_, std::defer_lock);
        const bool trackSymbols = !allSeen_.load(std::memory_order_relaxed);

        if (trackSymbols) {
            seenLock.lock();
        }

        const auto take = [&](std::int64_t packed, std::int32_t nanoPart, const std::string &symbol) {
            const auto lag = eventLagNanos(now, packed, nanoPart);

            lags.add(lag);
            values.push_back(lag);

            if (trackSymbols) {
                markSeen(symbol);
            }

            if (nanoPending) {
                nanoParts.push_back(nanoPart);
            }
        };

        // The events of another class are skipped.
        for (const auto &event : events) {
            if (kind_ == EventKind::TRADE) {
                if (const auto *trade = dynamic_cast<const Trade *>(event.get()); trade != nullptr) {
                    take(trade->getTimeSequence(), trade->getTimeNanoPart(), trade->getEventSymbol());
                }
            } else if (const auto *sale = dynamic_cast<const TimeAndSale *>(event.get()); sale != nullptr) {
                take(sale->getIndex(), sale->getTimeNanoPart(), sale->getEventSymbol());
            }
        }

        if (trackSymbols) {
            seenLock.unlock();
        }

        if (lags.count == 0) {
            return;
        }

        state_->addRecords(index_, now, lags, values);

        if (nanoPending) {
            state_->checkNanoParts(nanoParts);
        }
    }
};

// One feed endpoint with its subscription to a slice of the symbols.
struct Consumer {
    std::string name;
    std::shared_ptr<DXEndpoint> endpoint{};
    std::shared_ptr<DXFeedSubscription> subscription{};

    explicit Consumer(std::string endpointName) : name(std::move(endpointName)) {
    }

    Consumer(const Consumer &) = delete;
    Consumer &operator=(const Consumer &) = delete;

    ~Consumer() {
        if (subscription) {
            try {
                subscription->close();
            } catch (...) {
                // The endpoint closes it anyway.
            }
        }

        closeEndpoint(endpoint);
    }

    // Creates the endpoint, the subscription for the event, its listener and then its symbols. The endpoint connects
    // later.
    void create(const Settings &settings, const std::shared_ptr<ConsumerState> &state, std::size_t index,
                const std::vector<std::string> &slice) {
        endpoint = buildEndpoint(settings, name);
        subscription = endpoint->getFeed()->createSubscription(settings.event == EventKind::TRADE ? Trade::TYPE
                                                                                                  : TimeAndSale::TYPE);

        auto listener = std::make_shared<ConsumerListener>(state, index, settings.event, slice);

        subscription->addEventListener([listener](const std::vector<std::shared_ptr<EventType>> &events) {
            listener->receive(events);
        });
        subscription->addSymbols(slice);
    }

    bool connect(const std::string &address, std::string &error) {
        return connectEndpoint(*endpoint, name, address, error);
    }
};

// The starvation watchdog of the consumer, checked by the waiting thread of the run.
class Watchdog {
    const RunHooks &hooks_;
    // The steady-clock time since which the connections are open, unset while there are none.
    std::optional<std::int64_t> connectedSince_{};

    public:
    explicit Watchdog(const RunHooks &hooks) : hooks_(hooks) {
    }

    // Returns the message of the starvation, or an empty string.
    std::string check(std::int64_t connections, std::int64_t lastRecordNanos) {
        const auto now = steadyNanos();

        if (connections <= 0) {
            connectedSince_.reset();

            return {};
        }

        if (!connectedSince_.has_value()) {
            connectedSince_ = now;
        }

        const auto toNanos = [](std::chrono::milliseconds time) {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
        };

        if (lastRecordNanos == 0) {
            if (now - *connectedSince_ >= toNanos(hooks_.noDataAfterConnect)) {
                return "no records for " + std::to_string(hooks_.noDataAfterConnect.count()) +
                       " ms after the connection, with " + std::to_string(connections) + " open connections";
            }

            return {};
        }

        if (now - std::max(lastRecordNanos, *connectedSince_) >= toNanos(hooks_.noDataAfterRecord)) {
            return "no records for " + std::to_string(hooks_.noDataAfterRecord.count()) +
                   " ms after the last record, with " + std::to_string(connections) + " open connections";
        }

        return {};
    }
};

// How often the waiting thread of the consumer checks the watchdog and the nanosecond part.
constexpr std::chrono::milliseconds WATCHDOG_PERIOD{200};

} // namespace

std::vector<std::string> consumerSlice(const std::vector<std::string> &symbols, std::int32_t instance,
                                       std::int32_t instances) {
    std::vector<std::string> slice;

    if (instances < 1 || instance < 1 || instance > instances) {
        return slice;
    }

    for (auto index = static_cast<std::size_t>(instance - 1); index < symbols.size();
         index += static_cast<std::size_t>(instances)) {
        slice.push_back(symbols[index]);
    }

    return slice;
}

std::int64_t eventLagNanos(std::int64_t nowNanos, std::int64_t packedTime, std::int32_t nanoPart) {
    const auto timeMillis = (packedTime >> 32) * 1000 + ((packedTime >> 22) & 0x3FF);

    return nowNanos - (timeMillis * 1'000'000 + nanoPart);
}

RunResult runConsumer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out,
                      const RunHooks &hooks, std::shared_ptr<LiveCounters> live) {
    RunResult result;
    const auto startEpochNs = systemNanos();
    const auto state =
        std::make_shared<ConsumerState>(liveCountersOf(settings, std::move(live)), isNanoTimeOn(settings));
    std::vector<std::unique_ptr<Consumer>> consumers;
    std::mutex failureMutex;
    std::string failure;
    RunStatus failureStatus = RunStatus::FAILED;

    const auto fail = [&](const std::string &message, RunStatus status) {
        {
            std::lock_guard<std::mutex> lock(failureMutex);

            if (failure.empty()) {
                failure = message;
                failureStatus = status;
            }
        }

        stop.request();
    };

    // The intervals of the summary and the lags of the measured ones; the reporter writes them, and they outlive it.
    ReportCollector collector;
    Histogram lagHistogram;
    LagTotals lagTotals;

    collector.warmupStart = [&state] {
        return state->firstRecordEpochNanos();
    };
    collector.complete = [&](IntervalRecord &interval) {
        const auto [lags, histogram] = state->takeInterval();

        if (lags.count == 0) {
            return;
        }

        // C++ divides toward zero, as the specification wants for a negative sum.
        interval.lagMeanNs = lags.sum / lags.count;
        interval.lagP50Ns = histogram->valueAtPercentile(50.0);
        interval.lagP99Ns = histogram->valueAtPercentile(99.0);
        interval.lagMaxNs = lags.max;

        if (!interval.warmup) {
            lagHistogram.add(*histogram);
            lagTotals.add(lags);
        }
    };

    // The reporter is joined before the endpoints close, on an exception too.
    StopControl reporterStop;
    JoinGuard reporter(reporterStop);

    try {
        // The endpoints are closed when this function returns, those created before a failed one too.
        for (std::int32_t index = 1; index <= settings.instances; ++index) {
            auto &consumer = *consumers.emplace_back(
                std::make_unique<Consumer>(settings.name + "." + std::to_string(index)));
            std::string error;

            // The listener comes before the symbols, and the endpoint connects last.
            consumer.create(settings, state, static_cast<std::size_t>(index - 1),
                            consumerSlice(*symbols, index, settings.instances));
            result.endpointNames.push_back(consumer.endpoint->getName());

            if (!consumer.connect(settings.address, error)) {
                fail(error, RunStatus::FAILED);

                break;
            }
        }

        if (failure.empty()) {
            std::vector<std::shared_ptr<DXEndpoint>> endpoints;

            for (const auto &consumer : consumers) {
                endpoints.push_back(consumer->endpoint);
            }

            reporter.threads.emplace_back([&, endpoints] {
                try {
                    reportUntilStopped(settings, state->live, endpoints, reporterStop, out, collector);
                } catch (const std::exception &e) {
                    fail(std::string("the report failed: ") + e.what(), RunStatus::FAILED);
                }
            });

            // The thread of the caller waits for the stop or the duration and wakes every WATCHDOG_PERIOD to check the
            // watchdog and the nanosecond part. The watchdog takes the connections that the reporter read last: the
            // connectors are read at most once per period.
            using Steady = std::chrono::steady_clock;
            const auto deadline = settings.durationMillis.has_value()
                                      ? std::optional<Steady::time_point>(
                                            Steady::now() + std::chrono::milliseconds(std::min(
                                                                *settings.durationMillis, MAX_WAIT_MILLIS)))
                                      : std::nullopt;
            Watchdog watchdog(hooks);

            while (!stop.stopped() && !state->nanoMissing()) {
                auto wait = WATCHDOG_PERIOD;

                if (deadline.has_value()) {
                    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - Steady::now());

                    if (left.count() <= 0) {
                        break;
                    }

                    wait = std::min(wait, left);
                }

                if (stop.waitFor(wait)) {
                    break;
                }

                if (const auto starved = watchdog.check(state->live.openConnections(), state->lastRecordNanos());
                    !starved.empty()) {
                    fail(starved, RunStatus::STARVED);
                }
            }

            // The last statistics, then the endpoints.
            reporter.join();
        }
    } catch (const std::exception &e) {
        fail(e.what(), RunStatus::FAILED);
    }

    reporter.join();

    {
        // The statistics of the connectors are read before the endpoints close.
        std::vector<std::shared_ptr<DXEndpoint>> endpoints;

        for (const auto &consumer : consumers) {
            endpoints.push_back(consumer->endpoint);
        }

        result.report = makeReport(startEpochNs, collector, endpoints);
    }

    // The listeners are not called after their endpoints have closed: the counters are final then.
    consumers.clear();

    for (const auto &counters : state->live.instances) {
        const auto records = counters.records.load(std::memory_order_relaxed);

        result.instanceRecords.push_back(records);
        result.records += records;
    }

    result.lags = state->lags();
    result.symbolsSeen = state->symbolsSeen.load();

    // The percentiles, the maximum and the mean come from the measured intervals only.
    LagSummary lag;

    lag.count = lagTotals.count;
    lag.negative = lagTotals.negative;

    if (lag.count > 0) {
        lag.p50Ns = lagHistogram.valueAtPercentile(50.0);
        lag.p90Ns = lagHistogram.valueAtPercentile(90.0);
        lag.p99Ns = lagHistogram.valueAtPercentile(99.0);
        lag.p999Ns = lagHistogram.valueAtPercentile(99.9);
        lag.p9999Ns = lagHistogram.valueAtPercentile(99.99);
        lag.maxNs = lagTotals.max;
        lag.meanNs = lagTotals.sum / lagTotals.count;
    }

    result.report.e2eLag = lag;
    result.report.symbolsSeen = result.symbolsSeen;

    if (state->nanoMissing() && failure.empty()) {
        failure = "the first " + std::to_string(NANO_CHECK_EVENTS) +
                  " events have the nanosecond part 0: a hop of the path lacks dxscheme.nanoTime=true "
                  "(run with -Ddxscheme.nanoTime=false to receive without it)";
        failureStatus = RunStatus::NANO_MISSING;
    }

    if (!failure.empty()) {
        result.exitCode = 1;
        result.status = failureStatus;
        // The statuses of the consumer's checks lead the line; other failures keep the text of `p`.
        result.error = failureStatus == RunStatus::FAILED ? failure : runStatusName(failureStatus) + ": " + failure;
    }

    return result;
}

} // namespace dxfcpp::tools::nettest
