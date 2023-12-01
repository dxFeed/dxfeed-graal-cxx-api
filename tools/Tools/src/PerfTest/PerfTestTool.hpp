// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

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

#include <range/v3/all.hpp>

namespace dxfcpp::tools {

struct PerfTestTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<ArgType> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t) noexcept {
        return fmt::format("{:{}}{:<{}}{:{}}{}\n", "", namePadding, getFullName(), nameFieldSize - 2 * namePadding, "",
                           namePadding, SHORT_DESCRIPTION);
    }

    struct Diagnostic final {
      private:
        bool showCpuUsageByCore_{};
        std::chrono::milliseconds cpuStartTime_{};
        std::atomic<std::size_t> eventsCounter_{};
        std::atomic<std::size_t> listenerCallsCounter_{};

        mutable std::mutex mtx_{};
        double peakMemoryUsage_{};
        double peakCpuUsage_{};

        std::shared_ptr<Timer> timer_{};
        StopWatch timerDiff_{};
        StopWatch runningDiff_{};

        explicit Diagnostic(bool showCpuUsageByCore) noexcept : showCpuUsageByCore_{showCpuUsageByCore} {
            timerDiff_.restart();
            runningDiff_.restart();
            cpuStartTime_ = org::ttldtor::process::Process::getTotalProcessorTime();
        }

        static std::string formatDouble(double value) noexcept {
            return fmt::format("{:.2f}", std::isnan(value) ? 0.0 : value);
        }

        void onTimer() noexcept {
            auto eventsPerSecond = getEventsPerSecond();
            auto listenerCallsPerSecond = getListenerCallsPerSecond();

            std::lock_guard lock{mtx_};

            auto currentMemoryUsage = getMemoryUsage();
            peakMemoryUsage_ = currentMemoryUsage > peakMemoryUsage_ ? currentMemoryUsage : peakMemoryUsage_;

            auto currentCpuUsage = getCpuUsage();
            peakCpuUsage_ = currentCpuUsage > peakCpuUsage_ ? currentCpuUsage : peakCpuUsage_;

            fmt::print("\n{}\n", Platform::getPlatformInfo());
            std::cout << "----------------------------------------------------\n";
            fmt::print("  Rate of events (avg)           : {} (events/s)\n", formatDouble(eventsPerSecond));
            fmt::print("  Rate of listener calls         : {} (calls/s)\n", formatDouble(listenerCallsPerSecond));
            fmt::print("  Number of events in call (avg) : {} (events)\n",
                       formatDouble(eventsPerSecond / listenerCallsPerSecond));
            fmt::print("  Current memory usage           : {:.3f} (Mbyte)\n", currentMemoryUsage);
            fmt::print("  Peak memory usage              : {:.3f} (Mbyte)\n", peakMemoryUsage_);
            fmt::print("  Current CPU usage              : {:.2f} %\n", currentCpuUsage * 100.0);
            fmt::print("  Peak CPU usage                 : {:.2f} %\n", peakCpuUsage_ * 100.0);
            fmt::print("  Running time                   : {:%H:%M:%S}\n", runningDiff_.elapsed());

            timerDiff_.restart();
        }

        std::size_t getAndResetEventsCounter() noexcept {
            return eventsCounter_.exchange(0);
        }

        std::size_t getAndResetListenerCallsCounter() noexcept {
            return listenerCallsCounter_.exchange(0);
        }

        double getEventsPerSecond() noexcept {
            return static_cast<double>(getAndResetEventsCounter()) /
                   static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(timerDiff_.elapsed()).count());
        }

        double getListenerCallsPerSecond() noexcept {
            return static_cast<double>(getAndResetListenerCallsCounter()) /
                   static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(timerDiff_.elapsed()).count());
        }

        static double getMemoryUsage() noexcept {
            return static_cast<double>(org::ttldtor::process::Process::getWorkingSetSize()) / 1024.0 / 1024.0;
        }

        double getCpuUsage() noexcept {
            auto cpuEndTime = org::ttldtor::process::Process::getTotalProcessorTime();
            auto cpuDiff = cpuEndTime - cpuStartTime_;

            cpuStartTime_ = cpuEndTime;

            return static_cast<double>(cpuDiff.count()) /
                   (static_cast<double>(timerDiff_.elapsed().count()) *
                    static_cast<double>(!showCpuUsageByCore_ ? std::thread::hardware_concurrency() : 1));
        }

      public:
        static std::shared_ptr<Diagnostic> create(std::chrono::seconds measurementPeriod,
                                                  bool showCpuUsageByCore) noexcept {
            auto d = std::shared_ptr<Diagnostic>(new Diagnostic(showCpuUsageByCore));

            d->timer_ = Timer::schedule(
                [self = d] {
                    self->onTimer();
                },
                measurementPeriod, measurementPeriod);

            return d;
        }

        void addEventsCounter(std::size_t value) noexcept {
            eventsCounter_ += value;
        }

        void addListenerCallsCounter(std::size_t value) noexcept {
            listenerCallsCounter_ += value;
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

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            auto parsedAddress = AddressArgRequired::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            index++;

            auto parsedTypes = TypesArgRequired::parse(args);

            if (parsedTypes.isError) {
                return ParseResult<Args>::error(parsedTypes.errorString);
            }

            index++;

            auto parsedSymbols = SymbolsArgRequired::parse(args);

            if (parsedSymbols.isError) {
                return ParseResult<Args>::error(parsedSymbols.errorString);
            }

            index++;

            bool propertiesIsParsed{};
            std::optional<std::string> properties{};
            bool forceStream{};
            bool showCpuUsageByCore{};
            bool detachListener{};

            for (; index < args.size();) {
                if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                } else {
                    if (!forceStream && (forceStream = ForceStreamArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    if (!showCpuUsageByCore && (showCpuUsageByCore = CPUUsageByCoreArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    if (!detachListener && (detachListener = DetachListenerArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    index++;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, parsedTypes.result, parsedSymbols.result, properties,
                                          forceStream, showCpuUsageByCore, detachListener});
        }
    };

    static void run(const Args &args) noexcept {
        using namespace std::literals;

        auto endpoint = DXEndpoint::newBuilder()
                            ->withRole(args.forceStream ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default.
                            ->withProperties(CmdArgsUtils::parseProperties(args.properties))
                            ->withName(NAME + "Tool")
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
        endpoint->awaitNotConnected();
        endpoint->closeAndAwaitTermination();

        std::cout << hash << std::endl;
    }
};

} // namespace dxfcpp::tools