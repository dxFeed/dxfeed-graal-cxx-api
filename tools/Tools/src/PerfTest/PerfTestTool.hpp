// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

#include <process/process.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp::tools {

struct PerfTestTool {
    [[nodiscard]] std::string getName() const noexcept {
        return "PerfTest";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return "Connects to specified address and calculates performance counters.";
    }

    [[nodiscard]] std::string getDescription() const noexcept {
        return R"(
Connects to the specified address(es) and calculates performance counters (events per second, cpu usage, etc).
)";
    }

    [[nodiscard]] std::vector<std::string> getUsage() const noexcept {
        return {
            getName() + " <address> <types> <symbols> [<options>]",
        };
    }

    [[nodiscard]] std::vector<std::string> getAdditionalInfo() const noexcept {
        return {};
    }

    struct Diagnostic final {
      private:
        bool showCpuUsageByCore{};
        std::chrono::milliseconds cpuStartTime{};
        std::atomic<std::size_t> eventsCounter{};
        std::atomic<std::size_t> listenerCallsCounter{};

        double peakMemoryUsage{};
        double peakCpuUsage{};

        std::shared_ptr<Timer> timer{};
        StopWatch timerDiff{};
        StopWatch runningDiff{};

        explicit Diagnostic(bool showCpuUsageByCore) noexcept : showCpuUsageByCore{showCpuUsageByCore} {
            timerDiff.restart();
            runningDiff.restart();
            cpuStartTime = ttldtor::process::Process::getTotalProcessorTime();
        }

        void onTimer() noexcept {
            auto eventsPerSecond = getEventsPerSecond();
            auto listenerCallsPerSecond = getListenerCallsPerSecond();

            auto currentMemoryUsage = getMemoryUsage();
            peakMemoryUsage = currentMemoryUsage > peakMemoryUsage ? currentMemoryUsage : peakMemoryUsage;

            auto currentCpuUsage = getCpuUsage();
            peakCpuUsage = currentCpuUsage > peakCpuUsage ? currentCpuUsage : peakCpuUsage;

            std::cout << "\n----------------------------------------------------\n";
            fmt::print("  Rate of events (avg)           : {:.2f} (events/s)\n",
                       std::isnan(eventsPerSecond) ? 0.0 : eventsPerSecond);
            fmt::print("  Rate of listener calls         : {:.2f} (calls/s)\n",
                       std::isnan(listenerCallsPerSecond) ? 0.0 : listenerCallsPerSecond);

            auto numberOfEventsInCall = eventsPerSecond / listenerCallsPerSecond;

            fmt::print("  Number of events in call (avg) : {:.2f} (events)\n",
                       std::isnan(numberOfEventsInCall) ? 0.0 : numberOfEventsInCall);
            fmt::print("  Current memory usage           : {:.3f} (Mbyte)\n", currentMemoryUsage);
            fmt::print("  Peak memory usage              : {:.3f} (Mbyte)\n", peakMemoryUsage);
            fmt::print("  Current CPU usage              : {:.2f} %\n", currentCpuUsage * 100.0);
            fmt::print("  Peak CPU usage                 : {:.2f} %\n", peakCpuUsage * 100.0);
            fmt::print("  Running time                   : {:%H:%M:%S}\n", runningDiff.elapsed());

            timerDiff.restart();
        }

        std::size_t getAndResetEventsCounter() noexcept {
            return eventsCounter.exchange(0);
        }

        std::size_t getAndResetListenerCallsCounter() noexcept {
            return listenerCallsCounter.exchange(0);
        }

        double getEventsPerSecond() noexcept {
            return static_cast<double>(getAndResetEventsCounter()) /
                   static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(timerDiff.elapsed()).count());
        }

        double getListenerCallsPerSecond() noexcept {
            return static_cast<double>(getAndResetListenerCallsCounter()) /
                   static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(timerDiff.elapsed()).count());
        }

        static double getMemoryUsage() noexcept {
            return static_cast<double>(ttldtor::process::Process::getWorkingSetSize()) / 1024.0 / 1024.0;
        }

        double getCpuUsage() noexcept {
            auto cpuEndTime = ttldtor::process::Process::getTotalProcessorTime();
            auto cpuDiff = cpuEndTime - cpuStartTime;

            cpuStartTime = cpuEndTime;

            return static_cast<double>(cpuDiff.count()) /
                   (static_cast<double>(timerDiff.elapsed().count()) *
                    static_cast<double>(!showCpuUsageByCore ? std::thread::hardware_concurrency() : 1));
        }

      public:
        static std::shared_ptr<Diagnostic> create(std::chrono::seconds measurementPeriod,
                                                  bool showCpuUsageByCore) noexcept {
            auto d = std::shared_ptr<Diagnostic>(new Diagnostic(showCpuUsageByCore));

            d->timer = Timer::schedule(
                [self = d] {
                    self->onTimer();
                },
                measurementPeriod, measurementPeriod);

            return d;
        }

        void addEventsCounter(std::size_t value) noexcept {
            eventsCounter += value;
        }

        void addListenerCallsCounter(std::size_t value) noexcept {
            listenerCallsCounter += value;
        }
    };

    struct Args {
        std::string address{};
        std::string types{};
        std::string symbols{};
        std::optional<std::string> properties{};
        bool forceStream{};
        bool showCpuUsageByCore{};
        bool detachListener{};

        static Args parse(const std::vector<std::string> &args) noexcept {
            return {};
        }
    };

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;

        auto endpoint = DXEndpoint::newBuilder()
                            ->withRole(args.forceStream ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                            ->withProperties(CmdArgsUtils::parseProperties(args.properties))
                            ->withName(getName() + "Tool")
                            ->build();

        auto sub = endpoint->getFeed()->createSubscription(CmdArgsUtils::parseTypes(args.types));
        auto diagnostic = Diagnostic::create(2s, args.showCpuUsageByCore);

        std::atomic<std::size_t> hash{};

        if (!args.detachListener) {
            sub->addEventListener([d = diagnostic, &hash](auto &&events) {
                d->addListenerCallsCounter(1);
                d->addEventsCounter(events.size());

                for (auto &&e : events) {
                    hash += std::hash<std::shared_ptr<EventType>>{}(e);
                }
            });
        }

        sub->addSymbols(CmdArgsUtils::parseSymbols(args.symbols));
        endpoint->connect(args.address);
        endpoint->awaitNotConnected(); // non blocking
        endpoint->closeAndAwaitTermination();

        std::cout << hash << std::endl;
    }
};

} // namespace dxfcpp::tools