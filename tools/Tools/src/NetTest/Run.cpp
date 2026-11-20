// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Run.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "EventPool.hpp"
#include "Generator.hpp"
#include "RateLimiter.hpp"

#include <algorithm>
#include <csignal>
#include <exception>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
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
        if (endpoint) {
            try {
                endpoint->closeAndAwaitTermination();
            } catch (...) {
                // The endpoint is gone anyway.
            }
        }
    }

    // Builds the endpoint: the role, the name, the -D properties and the value of dxscheme.nanoTime on its builder.
    void create(const Settings &settings) {
        auto builder = DXEndpoint::newBuilder()->withRole(apiRole(endpointRole(settings)))->withName(name);

        for (const auto &[key, value] : settings.properties) {
            builder->withProperty(key, value);
        }

        builder->withProperty(NANO_TIME_PROPERTY, endpointNanoTime(settings));
        endpoint = builder->build();
        publisher = endpoint->getPublisher();
    }

    // Whether every server socket connector of the endpoint accepts connections.
    bool serverSocketsBound() const {
        for (const auto &connector : endpoint->getConnectors()) {
            if (connector.kind == MessageConnectorInfo::Kind::SERVER_SOCKET &&
                connector.state != MessageConnectorInfo::State::CONNECTED) {
                return false;
            }
        }

        return true;
    }

    // Connects the endpoint. A server socket must bind: the connector of a bound server socket reports itself as
    // connected within milliseconds, one that cannot bind (the port is taken) stays connecting, so the start waits until
    // every server socket connector is connected, at most SERVER_START_TIMEOUT. A client address never waits: with no
    // peer the connector stays connecting, which is a normal state. Returns false and fills `error` on a timeout.
    bool connect(const std::string &address, std::string &error) {
        endpoint->connect(address);

        const auto deadline = std::chrono::steady_clock::now() + SERVER_START_TIMEOUT;

        while (!serverSocketsBound()) {
            if (std::chrono::steady_clock::now() >= deadline) {
                error = "the server socket '" + address + "' (" + name + ") did not start in " +
                        std::to_string(SERVER_START_TIMEOUT.count()) + " ms (is the port already in use?)";

                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        return true;
    }
};

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

// The reporter thread: it lives until `stop` is requested, which comes after the generators have ended.
// TODO: MDAPI-600: print the statistics lines of every period from the live counters here, and the last block at the
// stop.
void reportUntilStopped(const Settings &, const LiveCounters &, StopControl &stop, std::ostream &) {
    stop.wait();
}

} // namespace

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
            std::unique_ptr<RateLimiter> limiter;

            if (settings.recordsPerSecond.has_value()) {
                limiter = std::make_unique<RateLimiter>(*settings.recordsPerSecond, settings.batch, stop);
            }

            reporter.threads.emplace_back([&] {
                try {
                    reportUntilStopped(settings, live, reporterStop, out);
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
                stop.waitFor(std::chrono::milliseconds(*settings.durationMillis));
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
        result.error = failure;
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
    // TODO: MDAPI-600: the consumer mode; the guard of the tool rejects it until then.
    if (settings.mode != Mode::PRODUCER) {
        err << "NetTest: the mode c is not implemented yet\n";

        return 1;
    }

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
    const auto result = runProducer(settings, pool.symbols, stop, out);

    if (result.exitCode != 0) {
        err << "NetTest: " << result.error << "\n";
    }

    return result.exitCode;
}

} // namespace dxfcpp::tools::nettest
