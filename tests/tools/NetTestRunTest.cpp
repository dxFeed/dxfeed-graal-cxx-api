// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The producer of NetTest: the endpoints, the schedule of the rate limiter, the stop and the exit codes.

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/NetTestTool.hpp"
#include "../../tools/Tools/src/NetTest/RateLimiter.hpp"
#include "../../tools/Tools/src/NetTest/Run.hpp"
#include "../../tools/Tools/src/NetTest/StopControl.hpp"
#include "../../tools/Tools/src/NetTest/SymbolPool.hpp"
#include "../../tools/Tools/src/ToolsProperties.hpp"
#include "../support/TestSupport.hpp"
#include "nettest/KitJson.hpp"

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