// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "../../../../src/internal/Isolate.hpp"
#include "../../../../src/internal/Platform.hpp"
#include "../../../../src/internal/utils/EnumUtils.hpp"
#include "../Args/Args.hpp"
#include "DiagnoseText.hpp"
#include "NetProbe.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dxfcpp::tools {

namespace diagnose {

/// The output of the tool: each line is redacted, printed and kept for the report file.
class Printer {
    std::mutex mutex_{};
    std::string report_{};

  public:
    void line(const std::string &text) {
        const auto safe = redact(text);
        std::lock_guard lock(mutex_);

        std::cout << safe << '\n';
        std::cout.flush();
        report_ += safe;
        report_ += '\n';
    }

    std::string report() {
        std::lock_guard lock(mutex_);

        return report_;
    }
};

inline std::int64_t nowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline std::string formatTime(std::int64_t millis) {
    return TimeFormat::DEFAULT_WITH_MILLIS.format(millis);
}

/// The statistics of the events of one type.
struct TypeStats {
    /// The maximum number of the first events that are printed.
    static constexpr std::size_t FIRST_EVENTS = 5;

    std::size_t count{};
    std::vector<std::string> first{};
    /// The time from the connect to the first event, ms.
    std::optional<std::int64_t> firstAfterMillis{};
    std::size_t snapshotBegin{};
    std::size_t snapshotEnd{};
    std::size_t txPending{};
    std::size_t removeEvent{};
    std::int64_t lagMin{std::numeric_limits<std::int64_t>::max()};
    std::int64_t lagMax{std::numeric_limits<std::int64_t>::min()};
    std::int64_t lagSum{};
    std::size_t lagCount{};
};

/// The last event of a type and symbol.
struct SymbolStats {
    std::size_t count{};
    std::int64_t lastMillis{};
};

/// What the event listeners collect; the listeners run on the threads of the library.
struct Collector {
    std::mutex mutex{};
    std::atomic<std::int64_t> connectMillis{};
    std::map<std::string, TypeStats> types{};
    std::map<std::pair<std::string, std::string>, SymbolStats> symbols{};
};

inline std::string eventSymbol(const std::shared_ptr<EventType> &event) {
    if (const auto withSymbol = std::dynamic_pointer_cast<EventTypeWithSymbol<std::string>>(event)) {
        return withSymbol->getEventSymbol();
    }

    if (const auto candle = std::dynamic_pointer_cast<Candle>(event)) {
        return candle->getEventSymbol().toString();
    }

    return "?";
}

/// The time of the event (ms), or 0 when the event has none.
inline std::int64_t eventTime(const std::shared_ptr<EventType> &event) {
    if (const auto time = event->getEventTime(); time != 0) {
        return time;
    }

    if (const auto quote = std::dynamic_pointer_cast<Quote>(event)) {
        return quote->getTime();
    }

    if (const auto trade = std::dynamic_pointer_cast<TradeBase>(event)) {
        return trade->getTime();
    }

    if (const auto timeSeries = std::dynamic_pointer_cast<TimeSeriesEvent>(event)) {
        return timeSeries->getTime();
    }

    return 0;
}

inline void collect(Collector &collector, const std::string &type,
                    const std::vector<std::shared_ptr<EventType>> &events) {
    const auto receiveMillis = nowMillis();
    std::lock_guard lock(collector.mutex);
    auto &stats = collector.types[type];

    if (!stats.firstAfterMillis && !events.empty()) {
        stats.firstAfterMillis = receiveMillis - collector.connectMillis.load();
    }

    for (const auto &event : events) {
        stats.count++;

        if (stats.first.size() < TypeStats::FIRST_EVENTS) {
            stats.first.push_back(event->toString());
        }

        if (const auto indexed = std::dynamic_pointer_cast<IndexedEvent>(event)) {
            const auto flags = static_cast<std::uint32_t>(indexed->getEventFlags());

            stats.snapshotBegin += IndexedEvent::SNAPSHOT_BEGIN.in(flags) ? 1U : 0U;
            stats.snapshotEnd += IndexedEvent::SNAPSHOT_END.in(flags) ? 1U : 0U;
            stats.txPending += IndexedEvent::TX_PENDING.in(flags) ? 1U : 0U;
            stats.removeEvent += IndexedEvent::REMOVE_EVENT.in(flags) ? 1U : 0U;
        }

        if (const auto time = eventTime(event); time > 0) {
            const auto lag = receiveMillis - time;

            stats.lagMin = std::min(stats.lagMin, lag);
            stats.lagMax = std::max(stats.lagMax, lag);
            stats.lagSum += lag;
            stats.lagCount++;
        }

        auto &symbol = collector.symbols[{type, eventSymbol(event)}];

        symbol.count++;
        symbol.lastMillis = receiveMillis;
    }
}

/// The result of the diagnosis of one address.
struct Result {
    std::string address{};
    Verdict verdict{};
    std::map<std::string, std::size_t> eventsPerType{};
    std::set<std::pair<std::string, std::string>> received{};
    bool wasConnected{};
};

/// The part of an address that is probed: the outcome of the DNS and TCP probes.
struct ProbeOutcome {
    bool anyHost{};
    bool anyResolved{};
    bool anyTcpConnected{};
};

/// The time of one TCP connect of the network probe.
constexpr std::chrono::milliseconds PROBE_TIMEOUT{3000};

inline ProbeOutcome probeAddress(Printer &printer, const std::string &address) {
    ProbeOutcome outcome{};

    printer.line("Network probe (DNS, then a TCP connect to each IP address, " +
                 std::to_string(PROBE_TIMEOUT.count()) + " ms each):");

    for (const auto &target : splitAddress(address)) {
        if (!target.skipped.empty()) {
            printer.line("  " + target.part + ": skipped, " + target.skipped);

            continue;
        }

        outcome.anyHost = true;

        const auto probe = probeHost(target.host, target.port, PROBE_TIMEOUT);
        const auto name = target.host + ":" + std::to_string(target.port);

        if (!probe.resolved) {
            printer.line("  " + name + ": DNS failed: " + probe.error);

            continue;
        }

        outcome.anyResolved = true;
        printer.line("  " + name + ": resolved to " + std::to_string(probe.ips.size()) + " IP address(es)");

        for (const auto &ip : probe.ips) {
            if (ip.connected) {
                outcome.anyTcpConnected = true;
                printer.line("    " + ip.ip + ": TCP connected in " + std::to_string(ip.millis) + " ms");
            } else {
                printer.line("    " + ip.ip + ": TCP failed after " + std::to_string(ip.millis) + " ms: " + ip.error);
            }
        }
    }

    if (!outcome.anyHost) {
        printer.line("  (no host:port part to probe)");
    }

    return outcome;
}

inline void printEvents(Printer &printer, Collector &collector, const std::vector<std::string> &types) {
    std::lock_guard lock(collector.mutex);

    printer.line("Events:");

    for (const auto &type : types) {
        const auto found = collector.types.find(type);

        if (found == collector.types.end() || found->second.count == 0) {
            printer.line("  " + type + ": no events");

            continue;
        }

        const auto &stats = found->second;

        printer.line(fmt::format("  {}: {} events, the first one {} ms after the connect", type, stats.count,
                                 stats.firstAfterMillis.value_or(0)));

        for (const auto &event : stats.first) {
            printer.line("    " + event);
        }

        if (stats.snapshotBegin + stats.snapshotEnd + stats.txPending + stats.removeEvent > 0) {
            printer.line(fmt::format("    flags: SNAPSHOT_BEGIN {}, SNAPSHOT_END {}, TX_PENDING {}, REMOVE_EVENT {}",
                                     stats.snapshotBegin, stats.snapshotEnd, stats.txPending, stats.removeEvent));
        }

        if (stats.lagCount > 0) {
            printer.line(fmt::format("    receive time - event time: min {} ms, avg {} ms, max {} ms", stats.lagMin,
                                     stats.lagSum / static_cast<std::int64_t>(stats.lagCount), stats.lagMax));
        } else {
            printer.line("    receive time - event time: n/a (the events have no time)");
        }
    }

    if (!collector.symbols.empty()) {
        printer.line("Last event per symbol:");

        for (const auto &[key, stats] : collector.symbols) {
            printer.line(fmt::format("  {} {}: {} ({} events)", key.first, key.second, formatTime(stats.lastMillis),
                                     stats.count));
        }
    }
}

inline std::string symbolName(const SymbolWrapper &symbol) {
    if (symbol.isWildcardSymbol()) {
        return "*";
    }

    if (symbol.isStringSymbol()) {
        return symbol.asStringSymbol();
    }

    return symbol.toStringUnderlying();
}

/// Diagnoses one address: the probe, the endpoint, the events and the verdict.
inline Result diagnoseOne(const std::shared_ptr<Printer> &sharedPrinter, const std::string &address,
                          const std::vector<std::reference_wrapper<const EventTypeEnum>> &types,
                          const std::vector<SymbolWrapper> &symbols,
                          const std::unordered_map<std::string, std::string> &properties, std::size_t durationSeconds) {
    auto &printer = *sharedPrinter;
    Result result{address};
    RunSummary summary{};

    for (const auto &type : types) {
        summary.types.push_back(type.get().getClassName());
    }

    for (const auto &symbol : symbols) {
        summary.symbols.push_back(symbolName(symbol));
    }

    printer.line("");
    printer.line("=== Address: " + address);

    const auto probe = probeAddress(printer, address);

    summary.noHostResolved = probe.anyHost && !probe.anyResolved;

    auto collector = std::make_shared<Collector>();
    auto wasConnected = std::make_shared<std::atomic<bool>>(false);
    std::optional<std::string> connectFailure{};

    printer.line("Endpoint:");

    auto endpoint = DXEndpoint::newBuilder()
                        ->withRole(DXEndpoint::Role::FEED)
                        ->withProperties(properties)
                        ->withName("DiagnoseTool")
                        ->build();

    // The listeners share the printer and the flags: they run on the threads of the library.
    endpoint->addStateChangeListener(
        [sharedPrinter, wasConnected](DXEndpoint::State oldState, DXEndpoint::State newState) {
            if (newState == DXEndpoint::State::CONNECTED) {
                *wasConnected = true;
            }

            sharedPrinter->line("  " + formatTime(nowMillis()) + " " + DXEndpoint::stateToString(oldState) + " -> " +
                                DXEndpoint::stateToString(newState));
        });

    try {
        endpoint->addMessageConnectorListener([sharedPrinter, lastCounts = std::map<std::string, std::int64_t>{}](
                                                  const MessageConnectorInfo &info) mutable {
            if (!info.lastError) {
                return;
            }

            // The count of the errors of a connector grows with each new error.
            auto &lastCount = lastCounts[info.name + " " + info.address];

            if (info.lastError->count > lastCount) {
                lastCount = info.lastError->count;
                sharedPrinter->line("  " + formatTime(nowMillis()) + " connection error: " +
                                    info.lastError->toString());
            }
        });
    } catch (const RuntimeException &e) {
        printer.line(std::string("  the connection errors are not printed: ") + e.what());
    }

    std::vector<std::shared_ptr<DXFeedSubscription>> subscriptions{};

    for (const auto &type : types) {
        auto subscription = endpoint->getFeed()->createSubscription(type.get());
        const auto name = type.get().getClassName();

        subscription->addEventListener([collector, name](const std::vector<std::shared_ptr<EventType>> &events) {
            collect(*collector, name, events);
        });
        subscription->addSymbols(symbols);
        subscriptions.push_back(std::move(subscription));
    }

    collector->connectMillis = nowMillis();

    try {
        endpoint->connect(address);
        std::this_thread::sleep_for(std::chrono::seconds(static_cast<std::int64_t>(durationSeconds)));
    } catch (const RuntimeException &e) {
        connectFailure = e.what();
        printer.line(std::string("  the address is not accepted: ") + e.what());
    }

    try {
        for (const auto &connector : endpoint->getConnectors()) {
            printer.line("  connector: " + connector.toString());
        }

        summary.lastError = endpoint->getLastConnectionError();
    } catch (const RuntimeException &e) {
        printer.line(std::string("  the connectors are not available: ") + e.what());
    }

    try {
        endpoint->closeAndAwaitTermination();
    } catch (const RuntimeException &e) {
        printer.line(std::string("  close failed: ") + e.what());
    }

    subscriptions.clear();

    printEvents(printer, *collector, summary.types);

    {
        std::lock_guard lock(collector->mutex);

        for (const auto &[key, stats] : collector->symbols) {
            summary.received.insert(key);
        }

        for (const auto &[type, stats] : collector->types) {
            result.eventsPerType[type] = stats.count;
        }
    }

    summary.wasConnected = *wasConnected;
    result.received = summary.received;
    result.wasConnected = summary.wasConnected;
    result.verdict = computeVerdict(summary);

    if (connectFailure && result.verdict.kind == VerdictKind::CANNOT_CONNECT) {
        result.verdict.hint = "The endpoint did not accept the address: " + *connectFailure;
    }

    printer.line("Verdict: " + std::string(verdictToString(result.verdict.kind)));

    if (!result.verdict.hint.empty()) {
        printer.line("Hint: " + result.verdict.hint);
    }

    for (const auto &missing : result.verdict.missing) {
        printer.line("  no events: " + missing);
    }

    if (probe.anyTcpConnected && !summary.wasConnected) {
        printer.line("Note: the TCP probe connected but the endpoint did not: often TLS, a login or a proxy.");
    }

    return result;
}

inline void printComparison(Printer &printer, const Result &first, const Result &second) {
    printer.line("");
    printer.line("=== Comparison");

    for (const auto *result : {&first, &second}) {
        std::size_t events = 0;

        for (const auto &[type, count] : result->eventsPerType) {
            events += count;
        }

        printer.line(fmt::format("  {}: {}, {} events, {}", result->address, verdictToString(result->verdict.kind),
                                 events, result->wasConnected ? "connected" : "not connected"));
    }

    std::set<std::string> types{};

    for (const auto *result : {&first, &second}) {
        for (const auto &[type, count] : result->eventsPerType) {
            types.insert(type);
        }
    }

    for (const auto &type : types) {
        const auto count = [&type](const Result &result) {
            const auto found = result.eventsPerType.find(type);

            return found == result.eventsPerType.end() ? std::size_t{0} : found->second;
        };

        printer.line(fmt::format("  {}: {} vs {}", type, count(first), count(second)));
    }

    const auto printOnlyIn = [&printer](const Result &has, const Result &lacks) {
        static constexpr std::size_t MAX_PAIRS = 20;
        std::size_t count = 0;

        for (const auto &pair : has.received) {
            if (lacks.received.count(pair) > 0) {
                continue;
            }

            if (++count <= MAX_PAIRS) {
                printer.line("  only from " + has.address + ": " + pair.first + " " + pair.second);
            }
        }

        if (count > MAX_PAIRS) {
            printer.line(fmt::format("  only from {}: {} more", has.address, count - MAX_PAIRS));
        }
    };

    printOnlyIn(first, second);
    printOnlyIn(second, first);

    printer.line(first.verdict.kind == second.verdict.kind ? "  The verdicts are the same."
                                                           : "  The verdicts differ.");
}

/// Prints the QD version and the QD log of the run (redacted), read from the log file.
inline void printQdLog(Printer &printer, const std::filesystem::path &logFile) {
    static constexpr std::size_t MAX_LOG_LINES = 200;
    static constexpr std::string_view VERSION_MARK = "Using QDS-";

    std::vector<std::string> lines{};
    std::optional<std::string> version{};
    // The QD logging writes to "<file>.0" (the first file of a rotated log).
    const auto rotatedFile = std::filesystem::path(logFile.string() + ".0");

    if (std::ifstream input(std::filesystem::exists(rotatedFile) ? rotatedFile : logFile); input) {
        for (std::string line; std::getline(input, line);) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (const auto mark = line.find(VERSION_MARK); !version && mark != std::string::npos) {
                const auto start = mark + VERSION_MARK.size();
                const auto end = line.find_first_of(" ,;", start);

                version = line.substr(start, end == std::string::npos ? std::string::npos : end - start);
            }

            lines.push_back(std::move(line));
        }
    }

    printer.line("");
    printer.line("=== QD");
    printer.line("QD: " + version.value_or("unknown (see the \"Using QDS-\" line of the log)"));

    if (lines.empty()) {
        return;
    }

    printer.line("QD log:");

    const auto skipped = lines.size() > MAX_LOG_LINES ? lines.size() - MAX_LOG_LINES : 0;

    if (skipped > 0) {
        printer.line(fmt::format("  ({} earlier lines are not shown)", skipped));
    }

    for (std::size_t i = skipped; i < lines.size(); i++) {
        printer.line("  " + lines[i]);
    }
}

inline constexpr std::string_view LOG_FILE_PREFIX = "dxfcxx-diagnose-";

/// Removes the QD log files of the run where the system allows it (on Windows they stay locked until the process ends).
inline void removeLogFiles(const std::filesystem::path &logFile) {
    for (const auto *suffix : {"", ".0", ".0.lck"}) {
        std::error_code ignored{};
        std::filesystem::remove(logFile.string() + suffix, ignored);
    }
}

/// Removes the QD log files of the run when it goes out of scope: on a return, an error and an exception alike.
struct LogFilesRemover {
    std::filesystem::path logFile;

    explicit LogFilesRemover(std::filesystem::path file) : logFile(std::move(file)) {
    }

    LogFilesRemover(const LogFilesRemover &) = delete;
    LogFilesRemover &operator=(const LogFilesRemover &) = delete;

    ~LogFilesRemover() {
        try {
            removeLogFiles(logFile);
        } catch (...) {
            // A file that is not removed now is removed by a later run.
        }
    }
};

/// Removes the QD log files that the earlier runs could not remove (older than an hour: not of a run in progress).
inline void removeOldLogFiles(const std::filesystem::path &directory) {
    using namespace std::chrono_literals;

    std::error_code error{};
    const auto now = std::filesystem::file_time_type::clock::now();

    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        if (it->path().filename().string().rfind(LOG_FILE_PREFIX, 0) != 0) {
            continue;
        }

        std::error_code timeError{};
        const auto time = std::filesystem::last_write_time(it->path(), timeError);

        if (!timeError && now - time > 1h) {
            std::error_code ignored{};
            std::filesystem::remove(it->path(), ignored);
        }
    }
}

} // namespace diagnose

struct DiagnoseTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<ArgType> ARGS;

    /// The time to collect the events of one address, s.
    static constexpr std::size_t DEFAULT_DURATION = 10;
    static constexpr std::size_t MAX_DURATION = 3600;

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

    struct Args {
        std::string address;
        std::string types;
        std::string symbols;
        std::optional<std::string> compare;
        std::size_t duration{DEFAULT_DURATION};
        std::optional<std::string> report;
        std::optional<std::string> properties;

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

            auto parsedTypes = TypesArgRequired<>::parse(args);

            if (parsedTypes.isError) {
                return ParseResult<Args>::error(parsedTypes.errorString);
            }

            index++;

            auto parsedSymbols = SymbolsArgRequired<>::parse(args);

            if (parsedSymbols.isError) {
                return ParseResult<Args>::error(parsedSymbols.errorString);
            }

            index++;

            bool compareIsParsed{};
            std::optional<std::string> compare{};
            bool durationIsParsed{};
            std::size_t duration{DEFAULT_DURATION};
            bool reportIsParsed{};
            std::optional<std::string> report{};
            bool propertiesIsParsed{};
            std::optional<std::string> properties{};

            while (index < args.size()) {
                if (!compareIsParsed && CompareArg::canParse(args, index)) {
                    auto parseResult = CompareArg::parse(args, index);

                    compare = parseResult.result;
                    compareIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!durationIsParsed && DurationArg::canParse(args, index)) {
                    auto parseResult = DurationArg::parse(args, index);

                    // A value that is not a number does not advance the index: it is an error, not a loop. A negative
                    // number wraps around to a large one and is out of the range too.
                    if (!parseResult.result || *parseResult.result == 0 || *parseResult.result > MAX_DURATION) {
                        return ParseResult<Args>::error(
                            fmt::format("The duration must be a whole number of seconds from 1 to {}: {}",
                                        MAX_DURATION, index + 1 < args.size() ? args[index + 1] : "no value"));
                    }

                    duration = *parseResult.result;
                    durationIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!reportIsParsed && ReportArg::canParse(args, index)) {
                    auto parseResult = ReportArg::parse(args, index);

                    report = parseResult.result;
                    reportIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                } else {
                    index++;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, parsedTypes.result, parsedSymbols.result, compare,
                                          duration, report, properties});
        }
    };

    /// Runs the diagnosis; returns the exit code: 0 for data, 2..5 by the worst verdict, 1 for an error.
    static int run(const Args &args) noexcept {
        using namespace diagnose;

        const auto sharedPrinter = std::make_shared<Printer>();
        auto &printer = *sharedPrinter;
        try {
            const auto logDirectory = std::filesystem::temp_directory_path();
            const auto logFile = logDirectory / fmt::format("{}{}.log", LOG_FILE_PREFIX, nowMillis());

            removeOldLogFiles(logDirectory);

            // Before anything that starts the Graal isolate (the parsing of the symbols does): the QD log of the run
            // goes to a file, read at the end. The files are removed on any way out (on Windows, QD keeps them open
            // until the process exits: a later run removes them).
            Logging::init(logFile.string(), Logging::Level::INFO);

            const LogFilesRemover logFilesRemover{logFile};
            const auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);

            System::setProperties(parsedProperties);

            auto [parsedTypes, unknownTypes] = CmdArgsUtils::parseTypes(args.types);

            if (!unknownTypes.empty()) {
                std::cerr << fmt::format("There are unknown event types: {}!\n List of available event types: {}\n",
                                         elementsToString(unknownTypes.begin(), unknownTypes.end(), "", ""),
                                         enum_utils::getEventTypeEnumClassNamesList(", "));

                return 1;
            }

            if (parsedTypes.empty()) {
                std::cerr << "The resulting list of types is empty!\n";

                return 1;
            }

            std::vector<std::reference_wrapper<const EventTypeEnum>> types(parsedTypes.begin(), parsedTypes.end());

            std::sort(types.begin(), types.end(), [](const auto &left, const auto &right) {
                return left.get().getClassName() < right.get().getClassName();
            });

            const auto symbols = CmdArgsUtils::parseSymbolsAndSaveOrder(args.symbols);

            printer.line(fmt::format("Diagnose {} {} {}", args.address, args.types, args.symbols));
            printer.line(fmt::format("Duration: {} s per address", args.duration));

            if (args.properties) {
                printer.line("Properties: " + *args.properties);
            }

            printer.line("dxFeed Graal C++ API: " DXFCXX_VERSION);
            printer.line("Graal Native SDK: " + Isolate::getGraalNativeSdkVersion());
            printer.line("Platform: " + Platform::getPlatformInfo());

            std::vector<Result> results{};

            results.push_back(
                diagnoseOne(sharedPrinter, args.address, types, symbols, parsedProperties, args.duration));

            if (args.compare) {
                results.push_back(
                    diagnoseOne(sharedPrinter, *args.compare, types, symbols, parsedProperties, args.duration));
                printComparison(printer, results[0], results[1]);
            }

            printQdLog(printer, logFile);

            auto worst = VerdictKind::DATA;

            for (const auto &result : results) {
                worst = std::max(worst, result.verdict.kind);
            }

            int code = exitCode(worst);

            if (args.report) {
                if (std::ofstream output(*args.report, std::ios::binary); output && (output << printer.report())) {
                    std::cout << "The report is written to " << *args.report << '\n';
                } else {
                    std::cerr << "The report is not written to " << *args.report << '\n';
                    code = 1;
                }
            }

            return code;
        } catch (const RuntimeException &e) {
            std::cerr << e << '\n';
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }

        return 1;
    }
};

} // namespace dxfcpp::tools
