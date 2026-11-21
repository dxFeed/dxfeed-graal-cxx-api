// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The runs of NetTest: the producer (the endpoints, the schedule of the rate limiter, the stop and the exit codes) and the
// consumer (the slices, the e2e lag, the watchdog and the check of the nanosecond part) over the loopback.

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/EventPool.hpp"
#include "../../tools/Tools/src/NetTest/NetTestTool.hpp"
#include "../../tools/Tools/src/NetTest/RateLimiter.hpp"
#include "../../tools/Tools/src/NetTest/Run.hpp"
#include "../../tools/Tools/src/NetTest/StopControl.hpp"
#include "../../tools/Tools/src/NetTest/SymbolPool.hpp"
#include "../../tools/Tools/src/ToolsProperties.hpp"
#include "../support/TestSupport.hpp"
#include "nettest/KitJson.hpp"
#include "nettest/KitSchema.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::tools;
using namespace dxfcpp::tools::nettest;
using namespace std::literals;

namespace {

// The process of the tests is the process of Tools: the same system properties before the first endpoint (the scheme
// is fixed there, so every test case sets the same values).
struct ToolsProcess {
    ToolsProcess() {
        setToolsSystemProperties();
    }
};

std::string listenAddress(std::uint16_t port) {
    return ":" + std::to_string(port) + "[bindAddr=127.0.0.1]";
}

// The settings of a command line that must be valid.
Settings settingsOf(const std::vector<std::string> &args) {
    const auto parsed = parseArguments(args);

    REQUIRE_MESSAGE(parsed.ok, parsed.error);

    return parsed.settings;
}

Symbols symbolsOf(const Settings &settings) {
    const auto pool = buildSymbolPool(settings);

    REQUIRE_MESSAGE(pool.ok, pool.error);

    return pool.symbols;
}

// A consumer endpoint that is connected to the port of a producer. A producer starts listening asynchronously and a
// feed refused before that reconnects only after 10-20 s, so the feed is created again until it is connected.
std::shared_ptr<DXEndpoint> connectedFeed(DXEndpoint::Role role, std::uint16_t port) {
    for (int attempt = 0; attempt < 20; ++attempt) {
        const auto endpoint = DXEndpoint::newBuilder()
                                  ->withRole(role)
                                  ->withName("NetTestRunFeed")
                                  ->withProperty(DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true")
                                  ->build();

        endpoint->connect("127.0.0.1:" + std::to_string(port));

        if (test::waitUntil(
                [&] {
                    return endpoint->getState() == DXEndpoint::State::CONNECTED;
                },
                1000ms)) {
            return endpoint;
        }

        endpoint->closeAndAwaitTermination();
    }

    return nullptr;
}

// Waits until a producer listens on the port: a consumer refused before that reconnects only after 10-20 s.
bool waitListening(std::uint16_t port) {
    const auto probe = connectedFeed(DXEndpoint::Role::STREAM_FEED, port);

    if (probe == nullptr) {
        return false;
    }

    probe->closeAndAwaitTermination();

    return true;
}

std::string clientAddress(std::uint16_t port) {
    return "127.0.0.1:" + std::to_string(port);
}

std::vector<std::string> linesOf(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);

    for (std::string line; std::getline(stream, line);) {
        lines.push_back(line);
    }

    return lines;
}

// The producer of the tool on a thread of the test, stopped and joined on every path.
struct BackgroundProducer {
    Settings settings;
    Symbols symbols;
    StopControl stop{};
    std::ostringstream out{};
    RunResult result{};
    std::thread thread{};

    explicit BackgroundProducer(const std::vector<std::string> &args, const RunHooks &hooks = {})
        : settings(settingsOf(args)), symbols(symbolsOf(settings)) {
        thread = std::thread([this, hooks] {
            result = runProducer(settings, symbols, stop, out, hooks);
        });
    }

    BackgroundProducer(const BackgroundProducer &) = delete;
    BackgroundProducer &operator=(const BackgroundProducer &) = delete;

    // Stops the producer and returns its result.
    const RunResult &finish() {
        stop.request();

        if (thread.joinable()) {
            thread.join();
        }

        return result;
    }

    ~BackgroundProducer() {
        finish();
    }
};

// The Trade events that a subscription of the symbol receives.
struct TradeCollector {
    std::mutex mutex{};
    std::vector<std::int32_t> nanoParts{};

    void add(const std::shared_ptr<Trade> &trade) {
        std::lock_guard lock{mutex};

        nanoParts.push_back(trade->getTimeNanoPart());
    }

    bool empty() {
        std::lock_guard lock{mutex};

        return nanoParts.empty();
    }

    std::vector<std::int32_t> snapshot() {
        std::lock_guard lock{mutex};

        return nanoParts;
    }
};

} // namespace

// The nanosecond part of the event time reaches a STREAM_FEED through a producer endpoint that gets
// `dxscheme.nanoTime` from its builder under the scheme of the Tools process.
TEST_CASE_FIXTURE(ToolsProcess,
                  "NetTest: the nano part passes a producer endpoint that gets dxscheme.nanoTime from its builder") {
    for (const auto role : {DXEndpoint::Role::PUBLISHER, DXEndpoint::Role::STREAM_PUBLISHER}) {
        const auto port = test::findFreeLoopbackPort();
        const auto publisherEndpoint = DXEndpoint::newBuilder()
                                           ->withRole(role)
                                           ->withName("NetTestRunPublisher")
                                           ->withProperty(DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true")
                                           ->build();

        publisherEndpoint->connect(listenAddress(port));
        REQUIRE(test::waitUntil([&] {
            return publisherEndpoint->getState() == DXEndpoint::State::CONNECTED;
        }));

        const auto feedEndpoint = connectedFeed(DXEndpoint::Role::STREAM_FEED, port);

        REQUIRE(feedEndpoint != nullptr);

        TradeCollector collector;
        const auto subscription = feedEndpoint->getFeed()->createSubscription(Trade::TYPE);

        subscription->addEventListener<Trade>([&](const auto &trades) {
            for (const auto &trade : trades) {
                collector.add(trade);
            }
        });
        subscription->addSymbols("NANO-TEST");

        const auto trade = std::make_shared<Trade>("NANO-TEST");

        trade->setTime(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
        trade->setTimeNanoPart(123456);
        trade->setPrice(10.5);

        // The subscription reaches the publisher asynchronously, and a trade published before that is not delivered.
        REQUIRE_MESSAGE(test::waitUntil(
                            [&] {
                                publisherEndpoint->getPublisher()->publishEvents(trade);

                                return !collector.empty();
                            },
                            10s, 100ms),
                        "no trade received from a "
                            << (role == DXEndpoint::Role::PUBLISHER ? "PUBLISHER" : "STREAM_PUBLISHER"));
        REQUIRE(collector.snapshot().front() == 123456);

        subscription->close();
        feedEndpoint->closeAndAwaitTermination();
        publisherEndpoint->closeAndAwaitTermination();
    }
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the producer publishes and stops on --duration") {
    const auto port = test::findFreeLoopbackPort();
    std::ostringstream out;
    std::ostringstream err;
    const auto start = std::chrono::steady_clock::now();
    const auto exitCode =
        NetTestTool::runProcess({"p", listenAddress(port), "-S", "100", "--duration", "2s"}, out, err);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK_MESSAGE(exitCode == 0, err.str());
    CHECK(err.str().empty());
    CHECK(elapsed >= 2s);
    CHECK(elapsed < 20s);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the producer keeps the rate of -r") {
    const auto port = test::findFreeLoopbackPort();
    const auto settings =
        settingsOf({"p", listenAddress(port), "-S", "100", "-r", "*/1000", "--batch", "100", "--duration", "3s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);
    // 10 batches a second for 3 s: the first batch comes at once, so 31 batches are due; the bounds are wide for a
    // slow machine, and the count is a whole number of batches.
    CHECK(result.records >= 2500);
    CHECK(result.records <= 3300);
    CHECK(result.records == result.batches * 100);
    REQUIRE(result.instanceRecords.size() == 1);
    CHECK(result.instanceRecords[0] == result.records);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: -C with the server address is a command line error") {
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess({"p", ":7001", "-S", "100", "-C", "2"}, out, err);

    CHECK(exitCode == 2);
    CHECK(err.str().rfind("NetTest: -C 2 cannot be used with the server address", 0) == 0);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: -C 2 creates the endpoints NetTest.1 and NetTest.2") {
    test::LoopbackTcpServer server;
    const auto settings = settingsOf({"p", server.getAddress(), "-S", "100", "-C", "2", "--duration", "1s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);
    CHECK(result.endpointNames == std::vector<std::string>{"NetTest.1", "NetTest.2"});
    REQUIRE(result.instanceRecords.size() == 2);
    CHECK(result.instanceRecords[0] > 0);
    CHECK(result.instanceRecords[1] > 0);
    CHECK(result.records == result.instanceRecords[0] + result.instanceRecords[1]);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the name of the endpoints follows -n") {
    test::LoopbackTcpServer server;
    const auto settings = settingsOf({"p", server.getAddress(), "-S", "10", "-n", "Load", "--duration", "500ms"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);
    CHECK(result.endpointNames == std::vector<std::string>{"Load.1"});
}

// Several endpoints publish from their own threads at the same time (the stress of the multi-instance run).
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: -C 4 publishes for 2 seconds from four threads") {
    // Nothing listens on the port: the connectors keep connecting, the events go to the endpoints all the same.
    const auto settings = settingsOf({"p", "127.0.0.1:" + std::to_string(test::findFreeLoopbackPort()), "-S", "1000",
                                      "-C", "4", "--duration", "2s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);
    REQUIRE(result.instanceRecords.size() == 4);

    for (const auto records : result.instanceRecords) {
        CHECK(records > 0);
    }

    CHECK(result.records > 0);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: -C 4 with a connected peer and a rate") {
    test::LoopbackTcpServer server;
    const auto settings =
        settingsOf({"p", server.getAddress(), "-S", "1000", "-C", "4", "-r", "*/20000", "--duration", "2s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);
    CHECK(test::waitUntil(
        [&] {
            return server.getAcceptedConnectionCount() >= 4;
        },
        5s));
    CHECK(result.records > 0);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: a busy listen port is exit 1 within 3 seconds") {
    // The server holds the port: the endpoint of the producer cannot bind it.
    test::LoopbackTcpServer server;
    const auto address = server.getAddress();
    const auto port = address.substr(address.rfind(':') + 1);
    std::ostringstream out;
    std::ostringstream err;
    const auto start = std::chrono::steady_clock::now();
    const auto exitCode =
        NetTestTool::runProcess({"p", ":" + port + "[bindAddr=127.0.0.1]", "-S", "10", "--duration", "30s"}, out, err);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(exitCode == 1);
    CHECK(err.str().find("NetTest: the server socket") == 0);
    CHECK(err.str().find("is the port already in use?") != std::string::npos);
    CHECK(elapsed < 10s);
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the system properties are set in the command line order") {
    const auto settings = settingsOf(
        {"p", ":7001", "-S", "5", "-Dnettest.test.key=first", "-Dnettest.test.key=second", "-Dnettest.test.other=x"});
    std::string error;

    REQUIRE(applySystemProperties(settings, error));
    CHECK(error.empty());
    CHECK(System::getProperty("nettest.test.key") == "second");
    CHECK(System::getProperty("nettest.test.other") == "x");
}

// The producer of the tool, a STREAM_FEED as the consumer: the events carry the nano part, and the stop request ends
// the run without a duration.
// With -Ddxscheme.nanoTime=false the producer endpoint drops the nano part: every event arrives with 0.
TEST_CASE_FIXTURE(ToolsProcess,
                  "NetTest: the events of the producer reach a STREAM_FEED with or without the nano part") {
    struct Case {
        std::vector<std::string> extra;
        bool nanoPart;
    };

    for (const auto &[extra, nanoPart] :
         {Case{{}, true}, Case{{"-c", "stream"}, true}, Case{{"-Ddxscheme.nanoTime=false"}, false},
          Case{{"-c", "stream", "-Ddxscheme.nanoTime=false"}, false}}) {
        const auto port = test::findFreeLoopbackPort();
        std::vector<std::string> args{"p", listenAddress(port), "-S", "20", "-r", "*/2000", "--batch", "20"};

        args.insert(args.end(), extra.begin(), extra.end());

        const auto settings = settingsOf(args);
        const auto symbols = symbolsOf(settings);
        StopControl stop;
        std::ostringstream out;
        RunResult result;
        std::thread producer([&] {
            result = runProducer(settings, symbols, stop, out);
        });

        const auto feedEndpoint = connectedFeed(DXEndpoint::Role::STREAM_FEED, port);

        if (feedEndpoint == nullptr) {
            stop.request();
            producer.join();
            FAIL("the feed did not connect to the producer");
        }

        TradeCollector collector;
        const auto subscription = feedEndpoint->getFeed()->createSubscription(Trade::TYPE);

        subscription->addEventListener<Trade>([&](const auto &trades) {
            for (const auto &trade : trades) {
                collector.add(trade);
            }
        });
        subscription->addSymbols(symbols->front());

        // Several events, so that "all are 0" is not one lucky event.
        const auto received = test::waitUntil(
            [&] {
                return collector.snapshot().size() >= 5;
            },
            15s);

        stop.request();
        producer.join();
        subscription->close();
        feedEndpoint->closeAndAwaitTermination();

        CHECK_MESSAGE(result.exitCode == 0, result.error);
        REQUIRE(received);

        std::int64_t nonZero = 0;

        for (const auto part : collector.snapshot()) {
            if (part != 0) {
                ++nonZero;
            }
        }

        if (nanoPart) {
            // The nano part of a batch is the time of the clock in nanoseconds within the millisecond: 0 by chance
            // only.
            CHECK(nonZero > 0);
        } else {
            CHECK(nonZero == 0);
        }
    }
}

TEST_CASE("RateLimiter: the batches are due at t0 + k * batch / rate") {
    StopControl stop;
    const RateLimiter limiter(1000, 100, stop);
    const auto start = RateLimiter::Clock::now();

    CHECK(limiter.due(start, 0) == start);
    CHECK(limiter.due(start, 1) == start + 100ms);
    CHECK(limiter.due(start, 25) == start + 2500ms);
}

TEST_CASE("RateLimiter: a generator that lags gets its batches at once and the slip grows") {
    StopControl stop;
    RateLimiter limiter(1000, 1, stop);

    REQUIRE(limiter.acquire().has_value());

    // 200 ms behind the schedule of 1 ms a batch.
    std::this_thread::sleep_for(200ms);

    const auto before = RateLimiter::Clock::now();
    std::vector<RateLimiter::Clock::duration> slips;

    for (int index = 0; index < 50; ++index) {
        const auto due = limiter.acquire();

        REQUIRE(due.has_value());
        slips.push_back(RateLimiter::Clock::now() - *due);
    }

    // 50 batches of the schedule are 50 ms long, and they are all in the past: no wait.
    CHECK(RateLimiter::Clock::now() - before < 150ms);

    for (const auto slip : slips) {
        CHECK(slip >= RateLimiter::Clock::duration::zero());
    }

    // The batch 1 was due at 1 ms, the batch 50 at 50 ms: the earlier a batch, the longer its slip.
    CHECK(slips.front() > slips.back());
    CHECK(slips.front() >= 140ms);
}

TEST_CASE("RateLimiter: the stop interrupts the wait") {
    StopControl stop;
    RateLimiter limiter(1, 1, stop);

    REQUIRE(limiter.acquire().has_value());

    std::thread stopper([&] {
        std::this_thread::sleep_for(100ms);
        stop.request();
    });
    const auto start = std::chrono::steady_clock::now();
    // The next batch is due in 1 s.
    const auto due = limiter.acquire();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    stopper.join();
    CHECK_FALSE(due.has_value());
    CHECK(elapsed < 800ms);
}

TEST_CASE("RateLimiter: the batches of all generators share one schedule") {
    StopControl stop;
    RateLimiter limiter(2000, 100, stop);
    std::atomic<int> granted{0};
    std::vector<std::thread> threads;

    for (int index = 0; index < 4; ++index) {
        threads.emplace_back([&] {
            for (int batch = 0; batch < 5; ++batch) {
                if (limiter.acquire().has_value()) {
                    granted.fetch_add(1);
                }
            }
        });
    }

    const auto start = std::chrono::steady_clock::now();

    for (auto &thread : threads) {
        thread.join();
    }

    // 20 batches of 50 ms: the last one is due at 950 ms, whichever thread takes it.
    CHECK(granted.load() == 20);
    CHECK(std::chrono::steady_clock::now() - start >= 600ms);
}

// The lines of the statistics of a real run pass the expressions of the kit: a block of the producer (the summary line,
// the line of each instance) and the runtime line after it, with the period of 1 s at the wall clock.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the statistics lines of a run pass the expressions of regexes.json") {
    const std::string directory = DXFCXX_NETTEST_KIT_DIR;
    const bool kitFound = std::filesystem::is_directory(directory);
#ifndef DXFCXX_NETTEST_SDK_STUB
    REQUIRE_MESSAGE(kitFound, "The kit of the NetTest specification is not found: " << directory);
#endif
    test::LoopbackTcpServer server;
    const auto settings =
        settingsOf({"p", server.getAddress(), "-S", "100", "-C", "2", "-s", "1s", "--duration", "3s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runProducer(settings, symbolsOf(settings), stop, out);

    CHECK_MESSAGE(result.exitCode == 0, result.error);

    std::vector<std::string> lines;
    std::istringstream text(out.str());

    for (std::string line; std::getline(text, line);) {
        lines.push_back(line);
    }

    // Three seconds at the period of 1 s: the first tick only starts the count, so two blocks of 3 lines (the summary
    // and 2 instances) with the runtime line, or three blocks when the last one is not shorter than a second.
    REQUIRE(lines.size() >= 2 * 4);
    CHECK(lines.size() % 4 == 0);
    CHECK(lines.size() <= 3 * 4);

    std::optional<std::regex> summary;
    std::optional<std::regex> instance;
    std::optional<std::regex> runtime;

    if (kitFound) {
        const auto expressions = kit::readJson(directory + "/regexes.json");

        summary.emplace(expressions["regexes"]["summary"].asString(), std::regex::ECMAScript);
        instance.emplace(expressions["regexes"]["instance"].asString(), std::regex::ECMAScript);
        runtime.emplace(expressions["regexes"]["runtime"].asString(), std::regex::ECMAScript);
    }

    for (std::size_t index = 0; index + 3 < lines.size(); index += 4) {
        INFO("block at line " << index);
        CHECK(lines[index].rfind("{*NetTestProducerSide-NetTest*} data ", 0) == 0);
        CHECK(lines[index + 1].rfind("    NetTest.1 data ", 0) == 0);
        CHECK(lines[index + 2].rfind("    NetTest.2 data ", 0) == 0);
        CHECK(lines[index + 3].rfind("{*NetTestRuntime-NetTest*} heap ", 0) == 0);

        if (kitFound) {
            CHECK_MESSAGE(std::regex_search(lines[index], *summary), lines[index]);
            CHECK_MESSAGE(std::regex_search(lines[index + 1], *instance), lines[index + 1]);
            CHECK_MESSAGE(std::regex_search(lines[index + 2], *instance), lines[index + 2]);
            CHECK_MESSAGE(std::regex_search(lines[index + 3], *runtime), lines[index + 3]);
        }
    }
}

namespace {

std::atomic<int> handledByTest{0};

extern "C" void handleInTest(int) {
    handledByTest.fetch_add(1);
}

} // namespace

TEST_CASE("NetTest: SIGINT and SIGTERM request the stop and the previous handlers come back") {
    for (const int signalNumber : {SIGINT, SIGTERM}) {
        INFO("signal " << signalNumber);

        const auto original = std::signal(signalNumber, handleInTest);

        {
            StopControl stop;
            const SignalWatcher watcher(stop);

            CHECK_FALSE(watcher.received());
            REQUIRE(std::raise(signalNumber) == 0);
            CHECK(stop.waitFor(5s));
            CHECK(watcher.received());
        }

        // The watcher took the signal instead of the handler of the test and gave that handler back.
        CHECK(handledByTest.load() == 0);
        CHECK(std::signal(signalNumber, original == SIG_ERR ? SIG_DFL : original) == handleInTest);
    }
}

TEST_CASE("NetTest: the e2e lag comes from the packed time and the nano part of the event") {
    // 2023-11-14T22:13:20.123456789Z: the packed time keeps the milliseconds, the nano part the rest.
    const std::int64_t eventNanos = 1'700'000'000'123'456'789;
    const auto packed = packTimeSequence(eventNanos, 7);
    const auto nanoPart = static_cast<std::int32_t>(eventNanos % 1'000'000);

    CHECK(eventLagNanos(eventNanos + 2'500, packed, nanoPart) == 2'500);
    CHECK(eventLagNanos(eventNanos, packed, nanoPart) == 0);
    // Without the nano part the event seems older by it.
    CHECK(eventLagNanos(eventNanos, packed, 0) == nanoPart);
    // A clock of the consumer behind the producer gives a negative lag.
    CHECK(eventLagNanos(eventNanos - 1'000, packed, nanoPart) == -1'000);

    // A negative lag enters the sum with its sign and the count of the negative ones.
    LagTotals lags;

    for (const std::int64_t lag : {2'500, -1'000, 300}) {
        lags.add(lag);
    }

    CHECK(lags.count == 3);
    CHECK(lags.sum == 1'800);
    CHECK(lags.negative == 1);
}

TEST_CASE("NetTest: the consumer instances share the symbols without overlap") {
    std::vector<std::string> symbols;

    for (int index = 0; index < 10; ++index) {
        symbols.push_back("S" + std::to_string(index));
    }

    CHECK(consumerSlice(symbols, 1, 3) == std::vector<std::string>{"S0", "S3", "S6", "S9"});
    CHECK(consumerSlice(symbols, 2, 3) == std::vector<std::string>{"S1", "S4", "S7"});
    CHECK(consumerSlice(symbols, 3, 3) == std::vector<std::string>{"S2", "S5", "S8"});
    CHECK(consumerSlice(symbols, 1, 1) == symbols);
    // More instances than symbols: the last ones get nothing.
    CHECK(consumerSlice({"A", "B"}, 3, 4).empty());
    CHECK(consumerSlice(symbols, 0, 3).empty());
    CHECK(consumerSlice(symbols, 4, 3).empty());
}

// The producer and the consumer of the tool in one process: the consumer of two instances receives every symbol, its
// mean e2e lag is positive and small, and the lines of both pass the expressions of the kit.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the consumer receives the events of the producer and measures the e2e lag") {
    const std::string directory = DXFCXX_NETTEST_KIT_DIR;
    const bool kitFound = std::filesystem::is_directory(directory);
#ifndef DXFCXX_NETTEST_SDK_STUB
    REQUIRE_MESSAGE(kitFound, "The kit of the NetTest specification is not found: " << directory);
#endif
#ifdef DXFCXX_SANITIZERS_ENABLED
    constexpr std::int64_t maxMeanLagNanos = 5'000'000'000;
#else
    constexpr std::int64_t maxMeanLagNanos = 1'000'000'000;
#endif
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "100", "-r", "*/10000", "-s", "1s"});

    REQUIRE(waitListening(port));

    const auto settings = settingsOf({"c", clientAddress(port), "-S", "100", "-C", "2", "-s", "1s", "--duration", "5s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runConsumer(settings, symbolsOf(settings), stop, out);
    const auto &produced = producer.finish();

    CHECK_MESSAGE(produced.exitCode == 0, produced.error);
    REQUIRE_MESSAGE(result.exitCode == 0, result.error);
    CHECK(result.status == RunStatus::OK);
    CHECK(result.endpointNames == std::vector<std::string>{"NetTest.1", "NetTest.2"});
    REQUIRE(result.instanceRecords.size() == 2);
    CHECK(result.instanceRecords[0] > 0);
    CHECK(result.instanceRecords[1] > 0);
    CHECK(result.symbolsSeen == 100);
    REQUIRE(result.records > 0);
    CHECK(result.lags.count == result.records);

    const auto meanLagNanos = result.lags.sum / result.lags.count;

    CHECK(meanLagNanos > 0);
    CHECK(meanLagNanos < maxMeanLagNanos);

    std::optional<std::regex> summary;
    std::optional<std::regex> instance;
    std::optional<std::regex> runtime;

    if (kitFound) {
        const auto expressions = kit::readJson(directory + "/regexes.json");

        summary.emplace(expressions["regexes"]["summary"].asString(), std::regex::ECMAScript);
        instance.emplace(expressions["regexes"]["instance"].asString(), std::regex::ECMAScript);
        runtime.emplace(expressions["regexes"]["runtime"].asString(), std::regex::ECMAScript);
    }

    const auto checkBlocks = [&](const std::vector<std::string> &lines, const std::string &side,
                                 std::size_t instances) {
        const auto blockSize = instances + 2;

        REQUIRE(lines.size() >= blockSize);
        CHECK(lines.size() % blockSize == 0);

        for (std::size_t index = 0; index + blockSize - 1 < lines.size(); index += blockSize) {
            INFO(side << " block at line " << index);
            CHECK(lines[index].rfind("{*NetTest" + side + "Side-NetTest*} data ", 0) == 0);

            for (std::size_t number = 1; number <= instances; ++number) {
                CHECK(lines[index + number].rfind("    NetTest." + std::to_string(number) + " data ", 0) == 0);
            }

            CHECK(lines[index + blockSize - 1].rfind("{*NetTestRuntime-NetTest*} heap ", 0) == 0);

            if (kitFound) {
                CHECK_MESSAGE(std::regex_search(lines[index], *summary), lines[index]);

                for (std::size_t number = 1; number <= instances; ++number) {
                    CHECK_MESSAGE(std::regex_search(lines[index + number], *instance), lines[index + number]);
                }

                CHECK_MESSAGE(std::regex_search(lines[index + blockSize - 1], *runtime), lines[index + blockSize - 1]);
            }
        }
    };

    checkBlocks(linesOf(out.str()), "Consumer", 2);
    checkBlocks(linesOf(producer.out.str()), "Producer", 1);
}

// The e2e lag starts at the clock read after the wait of the schedule: a producer held 50 ms ahead of its schedule waits
// that long before every batch, and the consumer's mean lag stays well below it.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the wait of a producer ahead of its schedule is not in the e2e lag") {
#ifdef DXFCXX_SANITIZERS_ENABLED
    // The delivery itself takes longer with the sanitizers.
    constexpr auto HOLD = 200ms;
#else
    constexpr auto HOLD = 50ms;
#endif
    const auto port = test::findFreeLoopbackPort();
    RunHooks hooks;

    hooks.holdAhead = HOLD;

    // Without the hold the schedule would grant a batch every millisecond.
    const auto start = std::chrono::steady_clock::now();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "100", "-r", "*/100k", "--batch", "100", "-s", "1s"},
                                hooks);

    REQUIRE(waitListening(port));

    const auto settings = settingsOf({"c", clientAddress(port), "-S", "100", "-s", "1s", "--duration", "5s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runConsumer(settings, symbolsOf(settings), stop, out);
    const auto &produced = producer.finish();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE_MESSAGE(produced.exitCode == 0, produced.error);
    REQUIRE_MESSAGE(result.exitCode == 0, result.error);
    INFO(out.str());

    // Every batch waited for the hold, and the slip does not show it.
    CHECK(produced.batches > 0);
    CHECK(produced.batches <= elapsed / HOLD + 1);
    REQUIRE(produced.report.slip.has_value());
    CHECK(produced.report.slip->maxNs < std::chrono::nanoseconds(HOLD).count());

    REQUIRE(result.records > 0);
    CHECK(result.lags.count == result.records);

    const auto meanLagNanos = result.lags.sum / result.lags.count;

    CHECK(meanLagNanos > 0);
    CHECK(meanLagNanos < std::chrono::nanoseconds(HOLD).count() / 2);
}

// The time of a TimeAndSale comes from its index: the consumer of --event TimeAndSale measures a positive small lag.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the consumer of TimeAndSale measures the e2e lag") {
#ifdef DXFCXX_SANITIZERS_ENABLED
    constexpr std::int64_t maxMeanLagNanos = 5'000'000'000;
#else
    constexpr std::int64_t maxMeanLagNanos = 1'000'000'000;
#endif
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "10", "--event", "TimeAndSale", "-r", "*/1000"});

    REQUIRE(waitListening(port));

    const auto settings =
        settingsOf({"c", clientAddress(port), "-S", "10", "--event", "TimeAndSale", "--duration", "3s"});
    StopControl stop;
    std::ostringstream out;
    const auto result = runConsumer(settings, symbolsOf(settings), stop, out);
    const auto &produced = producer.finish();

    CHECK_MESSAGE(produced.exitCode == 0, produced.error);
    REQUIRE_MESSAGE(result.exitCode == 0, result.error);
    CHECK(result.symbolsSeen == 10);
    REQUIRE(result.records > 0);
    CHECK(result.lags.count == result.records);

    const auto meanLagNanos = result.lags.sum / result.lags.count;

    CHECK(meanLagNanos > 0);
    CHECK(meanLagNanos < maxMeanLagNanos);
}

// With open connections and no records the watchdog stops the consumer: the events of the producer are of another type.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the watchdog stops the consumer without records after the connection") {
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "10", "--event", "TimeAndSale", "-r", "*/1000"});

    REQUIRE(waitListening(port));

    const auto settings = settingsOf({"c", clientAddress(port), "-S", "10", "--duration", "30s"});
    RunHooks hooks;

    hooks.noDataAfterConnect = 1000ms;

    StopControl stop;
    std::ostringstream out;
    const auto start = std::chrono::steady_clock::now();
    const auto result = runConsumer(settings, symbolsOf(settings), stop, out, hooks);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(result.exitCode == 1);
    CHECK(result.status == RunStatus::STARVED);
    CHECK(result.records == 0);
    CHECK_MESSAGE(result.error.rfind("no_data: no records for 1000 ms after the connection, with 1 open connections", 0) ==
                      0,
                  result.error);
    CHECK(elapsed < 20s);
}

// The records stop while the connection stays open: the watchdog stops the consumer after the last record.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the watchdog stops the consumer after the last record") {
    const auto port = test::findFreeLoopbackPort();
    const auto publisherEndpoint = DXEndpoint::newBuilder()
                                       ->withRole(DXEndpoint::Role::PUBLISHER)
                                       ->withName("NetTestRunPublisher")
                                       ->withProperty(DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "true")
                                       ->build();

    // Set by the publisher when the subscription of the consumer arrives; shared with the listener of the SDK.
    const auto subscribed = std::make_shared<std::atomic<bool>>(false);

    publisherEndpoint->getPublisher()->getSubscription(Trade::TYPE)
        ->addChangeListener(ObservableSubscriptionChangeListener::create(
            [subscribed](const std::unordered_set<SymbolWrapper> &) {
                subscribed->store(true);
            },
            [](const std::unordered_set<SymbolWrapper> &) {
            },
            [] {
            }));
    publisherEndpoint->connect(listenAddress(port));
    REQUIRE(test::waitUntil([&] {
        return publisherEndpoint->getState() == DXEndpoint::State::CONNECTED;
    }));

    // The watchdog sees the connections that the reporter read last: a short period keeps that delay below the silence.
    const auto settings = settingsOf({"c", clientAddress(port), "-S", "10", "-s", "1s", "--duration", "60s"});
    const auto symbols = symbolsOf(settings);
    RunHooks hooks;

    hooks.noDataAfterRecord = 1000ms;
    hooks.noDataAfterConnect = 30s;

    StopControl stop;
    std::ostringstream out;
    RunResult result;
    std::thread consumer([&] {
        result = runConsumer(settings, symbols, stop, out, hooks);
    });

    // Trades until 1 s after the subscription arrives (at most 20 s on a slow host), then nothing.
    const auto trade = std::make_shared<Trade>(symbols->front());
    const auto publishDeadline = std::chrono::steady_clock::now() + 20s;
    std::optional<std::chrono::steady_clock::time_point> publishEnd{};

    while (std::chrono::steady_clock::now() < publishDeadline &&
           (!publishEnd || std::chrono::steady_clock::now() < *publishEnd)) {
        if (!publishEnd && subscribed->load()) {
            publishEnd = std::chrono::steady_clock::now() + 1s;
        }

        const auto now =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        trade->setTimeSequence(packTimeSequence(now, 1));
        trade->setTimeNanoPart(static_cast<std::int32_t>(now % 1'000'000));
        publisherEndpoint->getPublisher()->publishEvents(trade);
        std::this_thread::sleep_for(50ms);
    }

    const auto silenceStart = std::chrono::steady_clock::now();

    consumer.join();

    const auto silence = std::chrono::steady_clock::now() - silenceStart;

    publisherEndpoint->closeAndAwaitTermination();

    CHECK(result.exitCode == 1);
    CHECK(result.status == RunStatus::STARVED);
    CHECK(result.records > 0);
    CHECK_MESSAGE(
        result.error.rfind("no_data: no records for 1000 ms after the last record, with 1 open connections", 0) == 0,
        result.error);
    CHECK(silence < 10s);
}

// A producer without the nano part: the consumer stops after the first 1000 events, unless it does not expect the nano
// part itself.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the consumer stops on events without the nano part") {
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer(
        {"p", listenAddress(port), "-S", "20", "-r", "*/5000", "--batch", "20", "-Ddxscheme.nanoTime=false"});

    REQUIRE(waitListening(port));

    SUBCASE("the default of the consumer is nano_missing and exit 1") {
        std::ostringstream out;
        std::ostringstream err;
        const auto start = std::chrono::steady_clock::now();
        const auto exitCode =
            NetTestTool::runProcess({"c", clientAddress(port), "-S", "20", "--duration", "30s"}, out, err);
        const auto elapsed = std::chrono::steady_clock::now() - start;

        CHECK(exitCode == 1);
        CHECK_MESSAGE(err.str().rfind("NetTest: nano_missing: the first 1000 events have the nanosecond part 0: a hop "
                                      "of the path lacks dxscheme.nanoTime=true (run with -Ddxscheme.nanoTime=false to "
                                      "receive without it)\n",
                                      0) == 0,
                      err.str());
        CHECK(elapsed < 20s);
    }

    SUBCASE("-Ddxscheme.nanoTime=false on the consumer receives to the end") {
        const auto settings = settingsOf(
            {"c", clientAddress(port), "-S", "20", "--duration", "3s", "-Ddxscheme.nanoTime=false"});
        StopControl stop;
        std::ostringstream out;
        const auto result = runConsumer(settings, symbolsOf(settings), stop, out);

        CHECK_MESSAGE(result.exitCode == 0, result.error);
        CHECK(result.status == RunStatus::OK);
        // More events than the check takes.
        CHECK(result.records > NANO_CHECK_EVENTS);
    }

    CHECK_MESSAGE(producer.finish().exitCode == 0, producer.result.error);
}

namespace {

// The summary file of a run, read and checked against the schema of the kit when the kit is there.
kit::Json readSummary(const std::string &file) {
    REQUIRE(std::filesystem::exists(file));
    CHECK_FALSE(std::filesystem::exists(file + ".tmp"));

    const auto text = kit::readText(file);
    const std::string directory = DXFCXX_NETTEST_KIT_DIR;

    if (std::filesystem::is_directory(directory)) {
        std::vector<std::string> errors;

        kit::checkSchema(kit::JsonReader(text).read(), kit::readJson(directory + "/nettest-summary.schema.json"), "$",
                         errors);

        std::string joined;

        for (const auto &error : errors) {
            joined += error + "; ";
        }

        INFO(text);
        CHECK_MESSAGE(errors.empty(), joined);
    }

    return kit::JsonReader(text).read();
}

// The records of the intervals of a summary.
std::int64_t intervalRecords(const kit::Json &summary) {
    std::int64_t records = 0;

    for (const auto &interval : summary["intervals"].array) {
        records += interval["records"].asInteger();
    }

    return records;
}

// A summary file in the temporary directory of the tests, removed before the test writes it.
std::string summaryFile(const std::string &name) {
    const auto file = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / name).string();

    std::filesystem::remove(file);

    return file;
}

} // namespace

// The consumer writes its summary file at the end of the run; the report of the producer gives the same summary. The
// warm-up of 1 s starts at the first batch (`p`) and the first record (`c`): the first intervals are warm-up, the last is
// measured, and the e2e lag counts the records of the measured intervals only.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest: the summaries of the producer and the consumer have the warm-up phases") {
#ifndef DXFCXX_NETTEST_SDK_STUB
    REQUIRE_MESSAGE(std::filesystem::is_directory(DXFCXX_NETTEST_KIT_DIR),
                    "The kit of the NetTest specification is not found: " << DXFCXX_NETTEST_KIT_DIR);
#endif
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "10", "-r", "*/1000", "-s", "1s", "--warmup", "1s"});

    REQUIRE(waitListening(port));

    const auto file = summaryFile("nettest-summary-c.json");
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess(
        {"c", clientAddress(port), "-S", "10", "-s", "1s", "--warmup", "1s", "--duration", "4s", "--summary", file}, out,
        err);
    const auto &produced = producer.finish();

    REQUIRE_MESSAGE(exitCode == 0, err.str());
    REQUIRE_MESSAGE(produced.exitCode == 0, produced.error);

    const auto consumer = readSummary(file);

    CHECK(consumer["side"].asString() == "consumer");
    CHECK(consumer["tool"]["implementation"].asString() == "cpp-api");
    CHECK(consumer["status"].asString() == "ok");
    CHECK(consumer["exit_code"].asInteger() == 0);
    CHECK(consumer["error"].isNull());
    CHECK(consumer["config"]["warmup_ms"].asInteger() == 1000);
    CHECK(consumer["symbols_seen"].asInteger() == 10);

    const auto &intervals = consumer["intervals"].array;

    REQUIRE(intervals.size() >= 2);
    CHECK(intervals.front()["phase"].asString() == "warmup");
    CHECK(intervals.back()["phase"].asString() == "measure");

    const auto records = consumer["records"].asInteger();
    const auto &lag = consumer["e2e_lag"];

    CHECK(records > 0);
    CHECK(intervalRecords(consumer) == records);
    CHECK(lag["count"].asInteger() > 0);
    CHECK(lag["count"].asInteger() < records);
    REQUIRE_FALSE(lag["p50_ns"].isNull());
    CHECK(lag["p50_ns"].asInteger() <= lag["p99_ns"].asInteger());
    REQUIRE(consumer["connectors"].array.size() == 1);
    CHECK(consumer["connectors"].array.at(0)["address"].asString() == clientAddress(port));
    CHECK(consumer["connectors"].array.at(0)["v1"]["read_data_records"].asInteger() > 0);

    // The producer: its report gives the summary that its run would write.
    const auto &report = produced.report;

    REQUIRE(report.intervals.size() >= 2);
    CHECK(report.intervals.front().warmup);
    CHECK_FALSE(report.intervals.back().warmup);
    CHECK(report.records == produced.records);
    REQUIRE(report.publish.has_value());
    CHECK(report.publish->calls == produced.batches);
    CHECK(report.publish->events == produced.records);
    CHECK(report.publish->nanos > 0);
    CHECK(report.slip.has_value());
    CHECK_FALSE(report.e2eLag.has_value());
    CHECK_FALSE(report.connectors.empty());

    const auto producerFile = summaryFile("nettest-summary-p.json");
    std::string error;

    REQUIRE_MESSAGE(writeSummary(producerFile, formatSummary(producer.settings, report, SummaryStatus{}), error),
                    error);

    const auto producerSummary = readSummary(producerFile);

    CHECK(producerSummary["side"].asString() == "producer");
    CHECK(intervalRecords(producerSummary) == producerSummary["records"].asInteger());
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: a failure before the run writes the summary with the status error") {
    const auto file = summaryFile("nettest-summary-before-run.json");
    const auto symbols = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-no-such.ipf").string();
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess({"p", ":7001", "-S", "10@" + symbols, "--summary", file}, out, err);

    CHECK(exitCode == 1);
    CHECK(err.str().find("NetTest: cannot read the IPF file") == 0);

    const auto summary = readSummary(file);

    CHECK(summary["status"].asString() == "error");
    CHECK(summary["exit_code"].asInteger() == 1);
    CHECK(summary["error"].asString().find("cannot read the IPF file") == 0);
    CHECK(summary["records"].asInteger() == 0);
    CHECK(summary["intervals"].array.empty());
}

TEST_CASE_FIXTURE(ToolsProcess, "NetTest: an endpoint that does not start gives the summary with the status error") {
    // The server holds the port: the endpoint of the producer cannot bind it.
    test::LoopbackTcpServer server;
    const auto address = server.getAddress();
    const auto port = address.substr(address.rfind(':') + 1);
    const auto file = summaryFile("nettest-summary-busy-port.json");
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess(
        {"p", ":" + port + "[bindAddr=127.0.0.1]", "-S", "10", "--duration", "30s", "--summary", file}, out, err);

    CHECK(exitCode == 1);

    const auto summary = readSummary(file);

    CHECK(summary["status"].asString() == "error");
    CHECK(summary["exit_code"].asInteger() == 1);
    CHECK(summary["error"].asString().find("the server socket") == 0);
    CHECK(summary["records"].asInteger() == 0);
    CHECK(summary["publish_call"]["calls"].asInteger() == 0);
}

TEST_CASE("NetTest: --summary into a directory that does not exist is a command line error") {
    const auto file = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-no-such-dir" / "summary.json").string();
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess({"c", "127.0.0.1:7001", "-S", "10", "--summary", file}, out, err);

    CHECK(exitCode == 2);
    CHECK(err.str().find("NetTest: --summary cannot write to the directory of '") == 0);
    CHECK_FALSE(std::filesystem::exists(file));
}
