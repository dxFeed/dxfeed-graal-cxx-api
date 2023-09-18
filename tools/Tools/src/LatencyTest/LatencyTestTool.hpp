// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <chrono>
#include <cstdint>
#include <list>
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

struct LatencyTest {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;

    struct Diagnostic final {
      private:
        std::atomic<std::size_t> eventsCounter_{};
        std::atomic<std::size_t> listenerCallsCounter_{};

        std::shared_ptr<Timer> timer_{};
        StopWatch timerDiff_{};
        StopWatch runningDiff_{};

        double min_ = math::NaN;
        double mean_ = math::NaN;
        double max_ = math::NaN;
        double percentile_ = math::NaN;
        double stdDev_ = math::NaN;
        double stdErr_ = math::NaN;

        std::size_t sampleSize_{};

        mutable std::mutex mtx_{};
        std::unordered_set<std::string> symbols_{};
        std::list<std::int64_t> deltaTimes_{};
        std::chrono::seconds measurementPeriod_{};

        explicit Diagnostic(std::chrono::seconds measurementPeriod) noexcept : measurementPeriod_{measurementPeriod} {
            timerDiff_.restart();
            runningDiff_.restart();
        }

        static std::string formatDouble(double value) noexcept {
            if (std::isnan(value)) {
                return "---";
            }

            return fmt::format("{:.2f}", value);
        }

        static double calcPercentile(std::vector<std::int64_t> sequence, double excelPercentile) noexcept {
            std::sort(std::begin(sequence), std::end(sequence));

            auto n = ((static_cast<double>(sequence.size()) - 1.0) * excelPercentile) + 1.0;

            if (std::abs(n - 1.0) < std::numeric_limits<double>::epsilon()) {
                return static_cast<double>(sequence[0]);
            }

            if (std::abs(static_cast<double>(sequence.size()) - 1.0) < std::numeric_limits<double>::epsilon()) {
                return static_cast<double>(sequence[sequence.size() - 1]);
            }

            auto k = static_cast<std::size_t>(n);
            auto d = n - static_cast<double>(k);

            return static_cast<double>(sequence[k - 1]) +
                   (d * (static_cast<double>(sequence[k]) - static_cast<double>(sequence[k - 1])));
        }

        static double calcMin(auto &&values) noexcept {
            return *std::min_element(std::begin(values), std::end(values));
        }

        static double calcMean(auto &&values) noexcept {
            return std::accumulate(std::begin(values), std::end(values), 0.0) / values.size();
        }

        static double calcMax(auto &&values) noexcept {
            return *std::max_element(std::begin(values), std::end(values));
        }

        static double calcStdDev(auto &&values) noexcept {
            double stdDev = 0.0;

            if (values.size() <= 1) {
                return stdDev;
            }

            double avg = calcMean(values);
            double sum = std::accumulate(std::begin(values), std::end(values), 0.0, [avg](double a, std::int64_t b) {
                return a + (static_cast<double>(b) - avg) * (static_cast<double>(b) - avg);
            });

            stdDev = std::sqrt(sum / (static_cast<double>(values.size()) - 1.0));

            return stdDev;
        }

        static double calcStdErr(auto &&values, double stdDev) noexcept {
            return stdDev / std::sqrt(static_cast<double>(values.size()));
        }

        void onTimer() noexcept {
            auto eventsPerSecond = getEventsPerSecond();
            auto listenerCallsPerSecond = getListenerCallsPerSecond();

            std::lock_guard lock{mtx_};

            if (!deltaTimes_.empty()) {
                min_ = calcMin(deltaTimes_);
                mean_ = calcMean(deltaTimes_);
                max_ = calcMax(deltaTimes_);
                percentile_ = calcPercentile(std::vector<std::int64_t>{deltaTimes_.begin(), deltaTimes_.end()}, 0.99);
                stdDev_ = calcStdDev(deltaTimes_);
                stdErr_ = calcStdErr(deltaTimes_, stdDev_);
                sampleSize_ = deltaTimes_.size();

                deltaTimes_.clear();
            }

            fmt::print("\n{}\n", Platform::getPlatformInfo());
            std::cout << "----------------------------------------------------\n";
            fmt::print("  Rate of events (avg)     : {} (events/s)\n", formatDouble(eventsPerSecond));
            fmt::print("  Rate of unique symbols   : {} (symbols/interval)\n", symbols_.size());
            fmt::print("  Min                      : {} (ms)\n", formatDouble(min_));
            fmt::print("  Max                      : {} (ms)\n", formatDouble(max_));
            fmt::print("  99th percentile          : {} (ms)\n", formatDouble(percentile_));
            fmt::print("  Mean                     : {} (ms)\n", formatDouble(mean_));
            fmt::print("  StdDev                   : {} (ms)\n", formatDouble(stdDev_));
            fmt::print("  Error                    : {} (ms)\n", formatDouble(stdErr_));
            fmt::print("  Sample size (N)          : {} (events)\n", sampleSize_);
            fmt::print("  Measurement interval     : {} (s)\n", measurementPeriod_.count());
            fmt::print("  Running time             : {:%H:%M:%S}\n", runningDiff_.elapsed());

            symbols_.clear();

            min_ = math::NaN;
            mean_ = math::NaN;
            max_ = math::NaN;
            percentile_ = math::NaN;
            stdDev_ = math::NaN;
            stdErr_ = math::NaN;
            sampleSize_ = 0;

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

      public:
        static std::shared_ptr<Diagnostic> create(std::chrono::seconds measurementPeriod) noexcept {
            auto d = std::shared_ptr<Diagnostic>(new Diagnostic(measurementPeriod));

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

        void handleEvents(const std::vector<std::shared_ptr<EventType>> &events) {
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
            std::size_t validEvents = 0;

            std::lock_guard lock{mtx_};

            for (auto &&e : events) {
                std::int64_t deltaTime = 0;

                if (std::shared_ptr<Quote> quote = e->sharedAs<Quote>(); quote) {
                    deltaTime = time - quote->getTime();
                    validEvents++;
                    deltaTimes_.emplace_back(deltaTime);
                    symbols_.emplace(quote->getEventSymbol());
                } else if (std::shared_ptr<Trade> trade = e->sharedAs<Trade>(); trade) {
                    deltaTime = time - trade->getTime();
                    validEvents++;
                    deltaTimes_.emplace_back(deltaTime);
                    symbols_.emplace(trade->getEventSymbol());
                } else if (std::shared_ptr<TradeETH> tradeEth = e->sharedAs<TradeETH>(); tradeEth) {
                    deltaTime = time - tradeEth->getTime();
                    validEvents++;
                    deltaTimes_.emplace_back(deltaTime);
                    symbols_.emplace(tradeEth->getEventSymbol());
                } else if (std::shared_ptr<TimeAndSale> timeAndSale = e->sharedAs<TimeAndSale>(); timeAndSale) {
                    if (!timeAndSale->isNew() || !timeAndSale->isValidTick()) {
                        continue;
                    }

                    deltaTime = time - timeAndSale->getTime();
                    validEvents++;
                    deltaTimes_.emplace_back(deltaTime);
                    symbols_.emplace(timeAndSale->getEventSymbol());
                }
            }

            addEventsCounter(validEvents);
        }
    };

    struct Args {
        std::string address{};
        std::optional<std::string> types{};
        std::optional<std::string> symbols{};
        std::optional<std::string> properties{};
        bool forceStream{};
        std::size_t interval{2};

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            auto parsedAddress = AddressArgRequired::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            auto parsedTypes = TypesArg::parse(args);
            auto parsedSymbols = SymbolsArg::parse(args);

            return ParseResult<Args>::ok({parsedAddress.result, parsedTypes.result, parsedSymbols.result, {}, false, 2});
        }
    };

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;

        auto endpoint = DXEndpoint::newBuilder()
                            ->withRole(args.forceStream ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default.
                            ->withProperties(CmdArgsUtils::parseProperties(args.properties))
                            ->withName(NAME + "Tool")
                            ->build();

        auto sub = endpoint->getFeed()->createSubscription(
            CmdArgsUtils::parseTypes(args.types.has_value() ? args.types.value() : "all"));
        auto diagnostic = Diagnostic::create(std::chrono::seconds(args.interval));

        sub->addEventListener([d = diagnostic](auto &&events) {
            d->addListenerCallsCounter(1);
            d->handleEvents(events);
        });

        sub->addSymbols(CmdArgsUtils::parseSymbols(args.symbols.has_value() ? args.symbols.value() : "all"));
        endpoint->connect(args.address);
        endpoint->awaitNotConnected();
        endpoint->closeAndAwaitTermination();
    }
};

} // namespace dxfcpp::tools