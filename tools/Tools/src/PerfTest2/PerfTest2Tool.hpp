// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/event/market/Profile.hpp"
#include "../../../../include/dxfeed_graal_cpp_api/event/market/Summary.hpp"
#include "../Args/Args.hpp"
#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <fmt/std.h>
#include <memory>
#include <process/process.hpp>
#include <variant>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wdeprecated-declarations")
#include <range/v3/all.hpp>
DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_MSC_WARNINGS_POP()

namespace dxfcpp::tools {

struct PerfTest2Tool {
    static inline auto NUMBER_OF_SYMBOLS_PROPERTY_NAME = "PerfTest2.numberOfSymbols";
    static inline auto SHUFFLE_NAME = "PerfTest2.shuffle";
    static inline auto FORMAT_NAME = "PerfTest2.format"; // csv, normal
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

        const bool dumpCsv_{};

        explicit Diagnostic(bool showCpuUsageByCore, bool dumpCsv) noexcept
            : showCpuUsageByCore_{showCpuUsageByCore}, dumpCsv_{dumpCsv} {
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

            if (dumpCsv_) {
                fmt::println("{},{},{},{},{:.3f},{:.3f},{:.2f},{:.2f},{:%H:%M:%S},{}", formatTimeStampFast(now()),
                             formatDouble(eventsPerSecond), formatDouble(listenerCallsPerSecond),
                             formatDouble(eventsPerSecond / listenerCallsPerSecond), currentMemoryUsage,
                             peakMemoryUsage_, currentCpuUsage * 100.0, peakCpuUsage_ * 100.0, runningDiff_.elapsed(),
                             ApiContext::getInstance()->getManager<MetricsManager>()->getAsI64("Entity.Event"));
            } else {
                fmt::print("\n{}\n", Platform::getPlatformInfo());
                std::cout << "----------------------------------------------------\n";
                fmt::print("  Rate of events (avg)           : {} (events/s)\n", formatDouble(eventsPerSecond));
                fmt::print("  Rate of listener calls         : {} (calls/s)\n", formatDouble(listenerCallsPerSecond));
                fmt::print("  Number of events in call (avg) : {} (events)\n",
                           formatDouble(eventsPerSecond / listenerCallsPerSecond));
                fmt::print("  Current memory usage           : {:.3f} (MByte)\n", currentMemoryUsage);
                fmt::print("  Peak memory usage              : {:.3f} (MByte)\n", peakMemoryUsage_);
                fmt::print("  Current CPU usage              : {:.2f} %\n", currentCpuUsage * 100.0);
                fmt::print("  Peak CPU usage                 : {:.2f} %\n", peakCpuUsage_ * 100.0);
                fmt::print("  Running time                   : {:%H:%M:%S}\n", runningDiff_.elapsed());
                std::cout << "----------------------------------------------------\n";
#if defined(DXFCXX_ENABLE_METRICS)
                fmt::print("{}\n", ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->dump());
#endif
            }

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
        static std::string getCsvHeader() {
            return "Timestamp,Rate of events (avg) [events/s],Rate of listener calls [calls/s],Number of events in "
                   "call (avg) [events],Current memory usage [MByte],Peak memory usage [MByte],Current CPU usage "
                   "[%],Peak CPU usage [%],Running time,Entity.Event";
        }

        static std::shared_ptr<Diagnostic> create(std::chrono::seconds measurementPeriod, bool showCpuUsageByCore,
                                                  bool dumpCsv) {
            auto d = std::shared_ptr<Diagnostic>(new Diagnostic(showCpuUsageByCore, dumpCsv));

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

            auto parsedAddress = AddressArgRequired<>::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            index++;

            auto parsedSymbols = SymbolsArgRequired<1>::parse(args);

            if (parsedSymbols.isError) {
                return ParseResult<Args>::error(parsedSymbols.errorString);
            }

            index++;

            bool propertiesIsParsed{};
            std::optional<std::string> properties{};
            bool forceStream{};
            bool showCpuUsageByCore{};
            bool detachListener{};

            while (index < args.size()) {
                if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                } else {
                    // ReSharper disable once CppUsingResultOfAssignmentAsCondition
                    if (!forceStream && (forceStream = ForceStreamArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    // ReSharper disable once CppUsingResultOfAssignmentAsCondition
                    if (!showCpuUsageByCore && (showCpuUsageByCore = CPUUsageByCoreArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    // ReSharper disable once CppUsingResultOfAssignmentAsCondition
                    if (!detachListener && (detachListener = DetachListenerArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    index++;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, parsedSymbols.result, properties, forceStream,
                                          showCpuUsageByCore, detachListener});
        }
    };

    static void run(const Args &args) noexcept {
        std::random_device rd;
        std::mt19937 gen{rd()};

        try {
            using namespace std::literals;

            auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);
            std::size_t numberOfSymbols = 10000; // -p PerfTest2.numberOfSymbols

            if (parsedProperties.contains(NUMBER_OF_SYMBOLS_PROPERTY_NAME)) {
                numberOfSymbols = std::stoull(parsedProperties[NUMBER_OF_SYMBOLS_PROPERTY_NAME]);
            }

            auto shuffle = false; // -p PerfTest2.shuffle

            if (parsedProperties.contains(SHUFFLE_NAME)) {
                shuffle = toBool(parsedProperties[SHUFFLE_NAME]);
            }

            auto dumpCsv = false;

            if (parsedProperties.contains(FORMAT_NAME)) {
                dumpCsv = iEquals(parsedProperties[FORMAT_NAME], "csv");
            }

            Logging::init();

            System::setProperties(parsedProperties);

            if (dumpCsv) {
                fmt::println("{}", Diagnostic::getCsvHeader());
            }

            auto parsedSymbols = CmdArgsUtils::parseSymbolsAndSaveOrder(args.symbols);

            if (parsedSymbols.size() > numberOfSymbols) {
                parsedSymbols.resize(numberOfSymbols);
            }

            if (shuffle) {
                ranges::shuffle(parsedSymbols, gen);
            }

            auto endpoint =
                DXEndpoint::newBuilder()
                    ->withRole(args.forceStream ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                    ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default.
                    ->withProperties(parsedProperties)
                    ->withName(NAME + "Tool-Feed")
                    ->build();

            auto sub = endpoint->getFeed()->createSubscription(
                //{Trade::TYPE, TradeETH::TYPE, Quote::TYPE, Summary::TYPE, Profile::TYPE, Candle::TYPE});
                {Trade::TYPE, TradeETH::TYPE, Quote::TYPE, Summary::TYPE, Profile::TYPE});
            auto sub2 = endpoint->getFeed()->createSubscription({TimeAndSale::TYPE});
            // auto sub3 = endpoint->getFeed()->createSubscription({Candle::TYPE});
            auto diagnostic = Diagnostic::create(2s, args.showCpuUsageByCore, dumpCsv);

            std::atomic<std::size_t> hash{};

            if (!args.detachListener) {
                sub->addEventListener([d = diagnostic, &hash](auto &&events) {
                    d->addListenerCallsCounter(1);
                    d->addEventsCounter(events.size());

                    for (auto &&e : events) {
                        hash += std::hash<std::shared_ptr<EventType>>{}(e);
                    }
                });

                sub2->addEventListener([d = diagnostic, &hash](auto &&events) {
                    d->addListenerCallsCounter(1);
                    d->addEventsCounter(events.size());

                    for (auto &&e : events) {
                        hash += std::hash<std::shared_ptr<EventType>>{}(e);
                    }
                });

                // sub3->addEventListener([d = diagnostic, &hash](auto &&events) {
                //     d->addListenerCallsCounter(1);
                //     d->addEventsCounter(events.size());
                //
                //     for (auto &&e : events) {
                //         hash += std::hash<std::shared_ptr<EventType>>{}(e);
                //     }
                // });
            }

            static auto fromTime = (std::chrono::milliseconds(now()) - std::chrono::days(3)).count();

            sub->addSymbols(parsedSymbols);
            sub->addSymbols(parsedSymbols | ranges::views::transform([](auto &&s) {
                                return TimeSeriesSubscriptionSymbol(
                                    CandleSymbol::valueOf(s.asStringSymbol(), {CandlePeriod::DAY}).toString(),
                                    fromTime);
                            }));
            sub2->addSymbols(parsedSymbols);
            // sub3->addSymbols(
            //     parsedSymbols | ranges::views::transform([](auto &&s) {
            //         return TimeSeriesSubscriptionSymbol(
            //             CandleSymbol::valueOf(s.asStringSymbol(), {CandlePeriod::valueOf(1,
            //             CandleType::MINUTE)}).toString(), fromTime);
            //     }));

            endpoint->connect(args.address);

            while (true) {
                std::cin.ignore();

                std::cerr << "============================= GC =============================" << std::endl;
                gc();
            }

            endpoint->awaitNotConnected();
            endpoint->closeAndAwaitTermination();

            std::cout << hash << std::endl;
        } catch (const RuntimeException &e) {
            std::cerr << e << '\n';
        }
    }
};

} // namespace dxfcpp::tools