// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Run.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "EventPool.hpp"
#include "Generator.hpp"
#include "RateLimiter.hpp"
#include "Statistics.hpp"

#include <algorithm>
#include <csignal>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dxfcpp::tools::nettest {

namespace {

DXEndpoint::Role apiRole(Role role) {
    switch (role) {
    case Role::FEED:
        return DXEndpoint::Role::FEED;
    case Role::STREAM_FEED:
        return DXEndpoint::Role::STREAM_FEED;
    case Role::PUBLISHER:
        return DXEndpoint::Role::PUBLISHER;
    case Role::STREAM_PUBLISHER:
        return DXEndpoint::Role::STREAM_PUBLISHER;
    }

    return DXEndpoint::Role::PUBLISHER;
}

// Builds an endpoint of the settings: the role, the name, the -D properties and the value of dxscheme.nanoTime on its
// builder.
std::shared_ptr<DXEndpoint> buildEndpoint(const Settings &settings, const std::string &name) {
    auto builder = DXEndpoint::newBuilder()->withRole(apiRole(endpointRole(settings)))->withName(name);

    for (const auto &[key, value] : settings.properties) {
        builder->withProperty(key, value);
    }

    builder->withProperty(NANO_TIME_PROPERTY, endpointNanoTime(settings));

    return builder->build();
}

// Closes the endpoint and waits until it has processed its data; does nothing without one.
void closeEndpoint(const std::shared_ptr<DXEndpoint> &endpoint) noexcept {
    if (endpoint) {
        try {
            endpoint->closeAndAwaitTermination();
        } catch (...) {
            // The endpoint is gone anyway.
        }
    }
}

// Whether every server socket connector of the endpoint accepts connections.
bool serverSocketsBound(const DXEndpoint &endpoint) {
    for (const auto &connector : endpoint.getConnectors()) {
        if (connector.kind == MessageConnectorInfo::Kind::SERVER_SOCKET &&
            connector.state != MessageConnectorInfo::State::CONNECTED) {
            return false;
        }
    }

    return true;
}

// Connects the endpoint. A server socket must bind: the connector of a bound server socket reports itself as connected
// within milliseconds, one that cannot bind (the port is taken) stays connecting, so the start waits until every server
// socket connector is connected, at most SERVER_START_TIMEOUT. A client address never waits: with no peer the connector
// stays connecting, which is a normal state. Returns false and fills `error` on a timeout.
bool connectEndpoint(DXEndpoint &endpoint, const std::string &name, const std::string &address, std::string &error) {
    endpoint.connect(address);

    const auto deadline = std::chrono::steady_clock::now() + SERVER_START_TIMEOUT;

    while (!serverSocketsBound(endpoint)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            error = "the server socket '" + address + "' (" + name + ") did not start in " +
                    std::to_string(SERVER_START_TIMEOUT.count()) + " ms (is the port already in use?)";

            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return true;
}

// One publisher endpoint with its event pool.
struct Instance {
    std::string name;
    std::shared_ptr<DXEndpoint> endpoint{};
    std::shared_ptr<DXPublisher> publisher{};
    EventPool pool;

    Instance(std::int32_t index, std::string endpointName, const Symbols &symbols, EventKind kind, bool reuse)
        : name(std::move(endpointName)), pool(index, symbols, kind, reuse) {
    }

    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;

    ~Instance() {
        closeEndpoint(endpoint);
    }

    void create(const Settings &settings) {
        endpoint = buildEndpoint(settings, name);
        publisher = endpoint->getPublisher();
    }

    bool connect(const std::string &address, std::string &error) {
        return connectEndpoint(*endpoint, name, address, error);
    }
};

std::int64_t systemNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::int64_t steadyNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// What the consumer instances share with their listeners, which run on the threads of the SDK: the live counters, the
// e2e lags, the symbols seen, the time of the last record for the watchdog and the check of the nanosecond part over the
// first events of all instances. The listeners hold it, so that a late call of a listener never outlives it.
class ConsumerState {
    std::mutex lagsMutex_{};
    LagTotals lags_{};
    std::mutex nanoMutex_{};
    std::int64_t nanoChecked_ = 0;
    bool nanoSeen_ = false;
    std::atomic<bool> nanoPending_;
    std::atomic<bool> nanoMissing_{false};
    // The steady-clock time of the last listener call with records, 0 before the first one.
    std::atomic<std::int64_t> lastRecordNanos_{0};

    public:
    LiveCounters live;
    // The distinct symbols received by all instances.
    std::atomic<std::int64_t> symbolsSeen{0};

    ConsumerState(std::int32_t instances, bool nanoCheck)
        : nanoPending_(nanoCheck), live(instances) {
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

    LagTotals lags() {
        std::lock_guard<std::mutex> lock(lagsMutex_);

        return lags_;
    }

    // Counts the records of one listener call of instance `index` (from 0).
    void addRecords(std::size_t index, const LagTotals &lags) {
        auto &counters = live.instances[index];

        counters.records.fetch_add(lags.count, std::memory_order_relaxed);
        counters.weightedNanos.fetch_add(lags.sum, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(lagsMutex_);

            lags_.add(lags);
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
        // Once every symbol of the slice came, the symbols are not looked up anymore.
        std::unique_lock<std::mutex> seenLock(seenMutex_, std::defer_lock);
        const bool trackSymbols = !allSeen_.load(std::memory_order_relaxed);

        if (trackSymbols) {
            seenLock.lock();
        }

        const auto take = [&](std::int64_t packed, std::int32_t nanoPart, const std::string &symbol) {
            lags.add(eventLagNanos(now, packed, nanoPart));

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

        state_->addRecords(index_, lags);

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

// Stops the waiting of `toRequest` and joins the threads: when the scope ends on any path, no thread outlives it.
struct JoinGuard {
    StopControl &toRequest;
    std::vector<std::thread> threads{};

    explicit JoinGuard(StopControl &control) : toRequest(control) {
    }

    JoinGuard(const JoinGuard &) = delete;
    JoinGuard &operator=(const JoinGuard &) = delete;

    void join() {
        toRequest.request();

        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        threads.clear();
    }

    ~JoinGuard() {
        join();
    }
};

// The runtime values of the isolate, or nothing when the isolate does not give them.
std::optional<RuntimeSample> readRuntime() {
    try {
        const auto statistics = System::getRuntimeStatistics();
        RuntimeSample sample;

        sample.heapUsedBytes = statistics.heapUsedBytes;
        sample.heapCommittedBytes = statistics.heapCommittedBytes;
        sample.youngGcCount = statistics.youngGcCount;
        sample.youngGcTimeMs = statistics.youngGcTimeMs;
        sample.fullGcCount = statistics.fullGcCount;
        sample.fullGcTimeMs = statistics.fullGcTimeMs;
        sample.processCpuTimeNs = statistics.processCpuTimeNs;

        return sample;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

// The open connections of all connectors of the endpoint.
std::int64_t connectionsOf(const DXEndpoint &endpoint) {
    std::int64_t total = 0;

    for (const auto &connector : endpoint.getConnectors()) {
        total += connector.connectionCount;
    }

    return total;
}

// The reporter thread: prints the statistics block and the runtime line of every period from the live counters, and
// the last block at the stop; it lives until `stop` is requested, which comes after the generators have ended (on `p`)
// or after the run has stopped (on `c`).
void reportUntilStopped(const Settings &settings, const LiveCounters &live,
                        const std::vector<std::shared_ptr<DXEndpoint>> &endpoints, StopControl &stop,
                        std::ostream &out) {
    runReporter(
        settings.name, settings.periodMillis, systemReporterClock(stop),
        [&] {
            StatisticsSample sample;

            for (std::size_t index = 0; index < endpoints.size(); ++index) {
                const auto &counters = live.instances[index];

                sample.instances.push_back({counters.records.load(std::memory_order_relaxed),
                                            counters.weightedNanos.load(std::memory_order_relaxed)});
                sample.connections += connectionsOf(*endpoints[index]);
            }

            sample.runtime = readRuntime();

            return sample;
        },
        [&](const std::string &text) {
            out << text << std::flush;
        },
        settings.mode == Mode::PRODUCER ? StatisticsSide::PRODUCER : StatisticsSide::CONSUMER);
}

} // namespace

std::string runStatusName(RunStatus status) {
    switch (status) {
    case RunStatus::OK:
        return "ok";
    case RunStatus::FAILED:
        return "error";
    case RunStatus::STARVED:
        return "no_data";
    case RunStatus::NANO_MISSING:
        return "nano_missing";
    }

    return "error";
}

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

bool applySystemProperties(const Settings &settings, std::string &error) {
    for (const auto &[key, value] : settings.properties) {
        try {
            if (!System::setProperty(key, value)) {
                error = "cannot set the system property " + key;

                return false;
            }
        } catch (const std::exception &e) {
            error = "cannot set the system property " + key + ": " + e.what();

            return false;
        }
    }

    return true;
}

RunResult runProducer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out) {
    RunResult result;
    LiveCounters live(settings.instances);
    std::vector<std::unique_ptr<Instance>> instances;
    std::mutex failureMutex;
    std::string failure;

    const auto fail = [&](const std::string &message) {
        {
            std::lock_guard<std::mutex> lock(failureMutex);

            if (failure.empty()) {
                failure = message;
            }
        }

        stop.request();
    };

    // The limiter outlives the generators that use it: the guards below join them before it is destroyed, on an
    // exception too.
    std::unique_ptr<RateLimiter> limiter;
    StopControl reporterStop;
    JoinGuard reporter(reporterStop);
    JoinGuard generators(stop);

    try {
        // The endpoints are closed when this function returns, those created before a failed one too.
        for (std::int32_t index = 1; index <= settings.instances; ++index) {
            auto instance = std::make_unique<Instance>(index, settings.name + "." + std::to_string(index), symbols,
                                                       settings.event, settings.reuse);
            auto &started = *instances.emplace_back(std::move(instance));
            std::string error;

            started.create(settings);
            result.endpointNames.push_back(started.endpoint->getName());

            if (!started.connect(settings.address, error)) {
                fail(error);

                break;
            }
        }

        if (failure.empty()) {
            if (settings.recordsPerSecond.has_value()) {
                limiter = std::make_unique<RateLimiter>(*settings.recordsPerSecond, settings.batch, stop);
            }

            std::vector<std::shared_ptr<DXEndpoint>> endpoints;

            for (const auto &instance : instances) {
                endpoints.push_back(instance->endpoint);
            }

            reporter.threads.emplace_back([&, endpoints = std::move(endpoints)] {
                try {
                    reportUntilStopped(settings, live, endpoints, reporterStop, out);
                } catch (const std::exception &e) {
                    fail(std::string("the report failed: ") + e.what());
                }
            });

            for (std::size_t index = 0; index < instances.size(); ++index) {
                generators.threads.emplace_back([&, index] {
                    try {
                        auto &instance = *instances[index];
                        const auto generated =
                            runGenerator(instance.publisher, instance.pool, static_cast<std::size_t>(settings.batch),
                                         limiter.get(), stop, live.instances[index], live.publish);

                        if (!generated.ok) {
                            fail("publishing failed: " + generated.error);
                        }
                    } catch (const std::exception &e) {
                        fail(std::string("publishing failed: ") + e.what());
                    }
                });
            }

            // The thread of the caller waits for the stop or the duration.
            if (settings.durationMillis.has_value()) {
                stop.waitFor(std::chrono::milliseconds(std::min(*settings.durationMillis, MAX_WAIT_MILLIS)));
            } else {
                stop.wait();
            }

            // The generators first, then the last statistics, then the endpoints.
            generators.join();
            reporter.join();
        }
    } catch (const std::exception &e) {
        fail(e.what());
    }

    generators.join();
    reporter.join();

    result.records = live.records();
    result.batches = live.publish.batches.load(std::memory_order_relaxed);

    for (const auto &counters : live.instances) {
        result.instanceRecords.push_back(counters.records.load(std::memory_order_relaxed));
    }

    instances.clear();

    if (!failure.empty()) {
        result.exitCode = 1;
        result.status = RunStatus::FAILED;
        result.error = failure;
    }

    return result;
}

RunResult runConsumer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out,
                      const RunHooks &hooks) {
    RunResult result;
    const auto state = std::make_shared<ConsumerState>(settings.instances, isNanoTimeOn(settings));
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
                    reportUntilStopped(settings, state->live, endpoints, reporterStop, out);
                } catch (const std::exception &e) {
                    fail(std::string("the report failed: ") + e.what(), RunStatus::FAILED);
                }
            });

            // The thread of the caller waits for the stop or the duration and wakes every WATCHDOG_PERIOD to check the
            // watchdog with the connections now, and the nanosecond part.
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

                std::int64_t connections = 0;

                for (const auto &endpoint : endpoints) {
                    connections += connectionsOf(*endpoint);
                }

                if (const auto starved = watchdog.check(connections, state->lastRecordNanos()); !starved.empty()) {
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
    // The listeners are not called after their endpoints have closed: the counters are final then.
    consumers.clear();

    for (const auto &counters : state->live.instances) {
        const auto records = counters.records.load(std::memory_order_relaxed);

        result.instanceRecords.push_back(records);
        result.records += records;
    }

    result.lags = state->lags();
    result.symbolsSeen = state->symbolsSeen.load();

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

namespace {

std::atomic<bool> signalReceived{false};

static_assert(std::atomic<bool>::is_always_lock_free, "the signal handler needs a lock-free flag");

extern "C" void onSignal(int) {
    signalReceived.store(true);
}

std::vector<int> watchedSignals() {
    std::vector<int> signals{SIGINT, SIGTERM};

#ifdef SIGBREAK
    signals.push_back(SIGBREAK);
#endif

    return signals;
}

} // namespace

SignalWatcher::SignalWatcher(StopControl &stop) : stop_(stop) {
    signalReceived.store(false);

    for (const int signalNumber : watchedSignals()) {
        previous_.emplace_back(signalNumber, std::signal(signalNumber, onSignal));
    }

    thread_ = std::thread([this] {
        while (!done_.load() && !stop_.stopped()) {
            if (signalReceived.load()) {
                stop_.request();

                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
}

bool SignalWatcher::received() const {
    return signalReceived.load();
}

SignalWatcher::~SignalWatcher() {
    done_.store(true);
    thread_.join();

    for (const auto &[signalNumber, handler] : previous_) {
        std::signal(signalNumber, handler == SIG_ERR ? SIG_DFL : handler);
    }
}

int run(const Settings &settings, std::ostream &out, std::ostream &err) {
    std::string error;

    // The system properties come before the first endpoint.
    if (!applySystemProperties(settings, error)) {
        err << "NetTest: " << error << "\n";

        return 1;
    }

    const auto pool = buildSymbolPool(settings);

    if (!pool.warning.empty()) {
        err << "NetTest: warning: " << pool.warning << "\n";
    }

    if (!pool.ok) {
        err << "NetTest: " << pool.error << "\n";

        return 1;
    }

    // Ctrl+C and SIGTERM end the run like --duration does: the endpoints close and the exit code is 0.
    StopControl stop;
    const SignalWatcher signalWatcher(stop);
    const auto result = settings.mode == Mode::PRODUCER ? runProducer(settings, pool.symbols, stop, out)
                                                        : runConsumer(settings, pool.symbols, stop, out);

    if (result.exitCode != 0) {
        err << "NetTest: " << result.error << "\n";
    }

    return result.exitCode;
}

} // namespace dxfcpp::tools::nettest
