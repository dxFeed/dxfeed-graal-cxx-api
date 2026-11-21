// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Run.hpp"

#include "EventPool.hpp"
#include "RateLimiter.hpp"
#include "RunSupport.hpp"

#include "../../../../src/internal/Isolate.hpp"

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

// The version of the tool: that of the C++ API without the leading `v` of its tag (`9.0.0`, as the other tools give
// theirs), unknown outside the build of the tool.
std::string toolVersion() {
#ifdef DXFCXX_VERSION
    std::string version = DXFCXX_VERSION;

    if (version.size() > 1 && version.front() == 'v') {
        version.erase(0, 1);
    }

    return version;
#else
    return {};
#endif
}

// Writes the summary file when --summary is given. Returns the exit code of the status, or 1 when the file cannot be
// written.
int finish(const Settings &settings, const RunReport &report, const SummaryStatus &status, std::ostream &err) {
    if (settings.summaryFile.empty()) {
        return status.exitCode;
    }

    std::string error;

    if (!writeSummary(settings.summaryFile, formatSummary(settings, report, status), error)) {
        err << "NetTest: cannot write the summary file '" << settings.summaryFile << "': " << error << "\n";

        return status.exitCode == 0 ? 1 : status.exitCode;
    }

    return status.exitCode;
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

std::string isolateRejectionLine(const std::vector<std::string> &arguments) {
    std::string line = "NetTest: the isolate rejected one of the options:";

    for (const auto &argument : arguments) {
        line += ' ';
        line += argument;
    }

    return line;
}

int createIsolate(const Settings &settings, std::ostream &err) {
    if (settings.isolateArguments.empty()) {
        return 0;
    }

    if (!System::setIsolateArguments(settings.isolateArguments)) {
        err << "NetTest: the isolate is already created, so its options cannot be applied\n";

        return 1;
    }

    // The first call into the isolate creates it with the arguments.
    static_cast<void>(Isolate::getInstance());

    const auto error = Isolate::getCreationError();

    if (error == CEntryPointErrorsEnum::NO_ERROR) {
        return 0;
    }

    // The reason that the isolate prints (GraalVM "Error: ...") stays above the line.
    if (error == CEntryPointErrorsEnum::ARGUMENT_PARSING_FAILED) {
        err << isolateRejectionLine(settings.isolateArguments) << "\n";

        // A rejected option is an error of the command line.
        return 2;
    }

    err << "NetTest: cannot create the isolate (error " << static_cast<std::uint32_t>(error) << ")\n";

    return 1;
}

RunResult runProducer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out,
                      const RunHooks &hooks, std::shared_ptr<LiveCounters> liveCounters) {
    RunResult result;
    const auto startEpochNs = systemNanos();
    const bool scheduled = settings.recordsPerSecond.has_value();
    const auto liveOwner = liveCountersOf(settings, std::move(liveCounters));
    auto &live = *liveOwner;
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
    // The intervals of the summary; the reporter writes them, and they outlive it.
    ReportCollector collector;
    std::int64_t lastPublishNanos = 0;

    collector.warmupStart = [&live] {
        return live.publish.firstBatchEpochNanos.load(std::memory_order_relaxed);
    };
    collector.complete = [&](IntervalRecord &interval) {
        const auto publishNanos = live.publish.fillAndPublishNanos();

        if (interval.records > 0) {
            interval.publishNsPerEvent = (publishNanos - lastPublishNanos) / interval.records;
        }

        lastPublishNanos = publishNanos;

        if (scheduled) {
            interval.slipMaxNs = live.publish.slipIntervalMaxNanos.exchange(0, std::memory_order_relaxed);
            interval.slipEndNs = live.publish.slipLastNanos.load(std::memory_order_relaxed);
        }
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
            if (scheduled) {
                limiter = std::make_unique<RateLimiter>(*settings.recordsPerSecond, settings.batch, stop);
            }

            std::vector<std::shared_ptr<DXEndpoint>> endpoints;

            for (const auto &instance : instances) {
                endpoints.push_back(instance->endpoint);
            }

            reporter.threads.emplace_back([&, endpoints = std::move(endpoints)] {
                try {
                    reportUntilStopped(settings, live, endpoints, reporterStop, out, collector);
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
                                         limiter.get(), stop, live.instances[index], live.publish, hooks.holdAhead);

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

    {
        // The statistics of the connectors are read before the endpoints close.
        std::vector<std::shared_ptr<DXEndpoint>> endpoints;

        for (const auto &instance : instances) {
            endpoints.push_back(instance->endpoint);
        }

        result.report = makeReport(startEpochNs, collector, endpoints);
    }

    result.report.publish = PublishSummary{result.batches, result.records, live.publish.fillAndPublishNanos()};

    if (scheduled) {
        result.report.slip =
            slipSummary(result.report.intervals, live.publish.slipLastNanos.load(std::memory_order_relaxed));
    }

    instances.clear();

    if (!failure.empty()) {
        result.exitCode = 1;
        result.status = RunStatus::FAILED;
        result.error = failure;
    }

    return result;
}

namespace {

std::atomic<bool> signalReceived{false};

static_assert(std::atomic<bool>::is_always_lock_free, "the signal handler needs a lock-free flag");

extern "C" void onSignal(int signalNumber) {
    // The second signal ends the process at once, as without the watcher: the run may wait for something that does
    // not check the stop (the symbols of a large or remote IPF file).
    if (signalReceived.exchange(true)) {
        std::signal(signalNumber, SIG_DFL);
        std::raise(signalNumber);

        return;
    }

    // Windows restores the default handler before calling this one.
    std::signal(signalNumber, onSignal);
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
    SummaryStatus status;
    RunReport failedReport;

    status.version = toolVersion();
    failedReport.startEpochNs = systemNanos();

    // A failure before the run: the message on stderr and the summary of the status `error`.
    const auto failBeforeRun = [&](const std::string &message) {
        err << "NetTest: " << message << "\n";
        failedReport.endEpochNs = systemNanos();
        status.status = runStatusName(RunStatus::FAILED);
        status.exitCode = 1;
        status.error = message;

        return finish(settings, failedReport, status, err);
    };

    std::string error;

    // The system properties come before the first endpoint.
    if (!applySystemProperties(settings, error)) {
        return failBeforeRun(error);
    }

    // Empty, so `null` in the summary, when the isolate could not be created.
    status.sdkVersion = Isolate::getGraalNativeSdkVersion();

    // An exception that the run does not handle itself (an allocation, a thread that does not start) ends it with the
    // summary of the status `error` and the exit code 1 too.
    try {
        // Ctrl+C and SIGTERM end the run like --duration does: the last statistics are printed, the summary is written,
        // the endpoints close and the exit code is 0. The watcher is installed once the isolate exists, so that a signal
        // while the symbols are read (a large IPF file) also ends with the summary; a second signal ends the process.
        StopControl stop;
        const SignalWatcher signalWatcher(stop);
        const bool producer = settings.mode == Mode::PRODUCER;
        // The server of /metrics reads only the live counters; declared after them, it stops first.
        const auto live = std::make_shared<LiveCounters>(settings.instances);
        std::unique_ptr<MetricsServer> metrics;

        // The /metrics port is bound before the first endpoint.
        if (settings.metricsPort.has_value()) {
            metrics = std::make_unique<MetricsServer>(settings.metricsHost, *settings.metricsPort);

            if (!metrics->ok()) {
                return failBeforeRun(metrics->error());
            }

            if (!metrics->loopback()) {
                err << "NetTest: warning: the metrics on " << settings.metricsHost << ":" << metrics->port()
                    << " have no authentication and no TLS\n";
            }

            MetricsInfo info;

            info.side = producer ? "producer" : "consumer";
            info.name = settings.name;
            info.version = status.version;
            info.sdkVersion = status.sdkVersion;
            info.role = roleName(endpointRole(settings));
            info.event = eventName(settings.event);
            info.collector = settings.collector.has_value() ? collectorName(*settings.collector) : "";
            info.scheduled = producer && settings.recordsPerSecond.has_value();
            metrics->start([info, live] {
                return formatMetrics(info, *live);
            });
            out << "{*NetTestMetrics-" << settings.name << "*} " << metrics->url() << std::endl;
        }

        const auto pool = buildSymbolPool(settings);

        if (!pool.warning.empty()) {
            err << "NetTest: warning: " << pool.warning << "\n";
        }

        if (!pool.ok) {
            return failBeforeRun(pool.error);
        }

        // A signal before the start of the endpoints: the summary of the empty run, without the endpoints.
        if (stop.stopped()) {
            failedReport.endEpochNs = systemNanos();
            status.status = "stopped_by_signal";
            status.exitCode = 0;

            return finish(settings, failedReport, status, err);
        }

        const auto result = producer ? runProducer(settings, pool.symbols, stop, out, {}, live)
                                     : runConsumer(settings, pool.symbols, stop, out, {}, live);

        if (result.exitCode != 0) {
            err << "NetTest: " << result.error << "\n";
        }

        status.status = result.status == RunStatus::OK && signalWatcher.received() ? std::string("stopped_by_signal")
                                                                                   : runStatusName(result.status);
        status.exitCode = result.exitCode;
        status.error = result.error;

        return finish(settings, result.report, status, err);
    } catch (const std::exception &e) {
        return failBeforeRun(e.what());
    }
}

} // namespace dxfcpp::tools::nettest
