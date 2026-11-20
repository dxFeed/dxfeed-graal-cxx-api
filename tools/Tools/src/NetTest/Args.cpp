// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Args.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>

namespace dxfcpp::tools::nettest {

namespace {

// The largest symbol set, batch and number of instances that the tool accepts. A batch shares one time, so it holds at
// most 2^22 events: the 22-bit sequence of the packed time must not wrap inside one batch.
constexpr std::int64_t MAX_SYMBOLS = 100'000'000;
constexpr std::int64_t MAX_BATCH = 1 << 22;
constexpr std::int64_t MAX_INSTANCES = 1'000;

bool isDigits(const std::string &text) {
    if (text.empty()) {
        return false;
    }

    for (const char symbol : text) {
        if (symbol < '0' || symbol > '9') {
            return false;
        }
    }

    return true;
}

// Parses a positive decimal integer of at most `limit`; nullopt otherwise.
std::optional<std::int64_t> parsePositive(const std::string &text, std::int64_t limit) {
    if (!isDigits(text) || text.size() > 18) {
        return std::nullopt;
    }

    const auto value = std::strtoll(text.c_str(), nullptr, 10);

    if (value < 1 || value > limit) {
        return std::nullopt;
    }

    return value;
}

// The options of the specification by their names. The long names of the short options are aliases; a name that
// starts with `--` also takes its value as `--name=value`.
enum class Option {
    SYMBOLS,
    INSTANCES,
    RATE,
    STAT,
    NAME,
    COLLECTOR,
    EVENT,
    BATCH,
    REUSE,
    ROLE,
    DURATION,
    WARMUP,
    SUMMARY,
    METRICS_PORT,
    LEGACY,
    HELP,
    WILDCARD,
};

struct OptionSpec {
    const char *name;
    Option option;
    // Takes a value (the next argument, or the text after `=`).
    bool hasValue;
    // An option of the mode p only: rejected on `c`.
    bool producerOnly;
};

constexpr OptionSpec OPTIONS[] = {
    {"-S", Option::SYMBOLS, true, false},
    {"-C", Option::INSTANCES, true, false},
    {"-r", Option::RATE, true, true},
    {"--rate-limit", Option::RATE, true, true},
    {"-s", Option::STAT, true, false},
    {"--stat", Option::STAT, true, false},
    {"-n", Option::NAME, true, false},
    {"--name", Option::NAME, true, false},
    {"-c", Option::COLLECTOR, true, false},
    {"--collector", Option::COLLECTOR, true, false},
    {"--event", Option::EVENT, true, false},
    {"--batch", Option::BATCH, true, true},
    {"--reuse", Option::REUSE, false, true},
    {"--role", Option::ROLE, true, false},
    {"--duration", Option::DURATION, true, false},
    {"--legacy", Option::LEGACY, false, false},
    {"--warmup", Option::WARMUP, true, false},
    {"--summary", Option::SUMMARY, true, false},
    {"--metrics-port", Option::METRICS_PORT, true, false},
    {"--help", Option::HELP, false, false},
    {"-w", Option::WILDCARD, false, false},
    {"--wildcard", Option::WILDCARD, false, false},
};

const OptionSpec *findOption(const std::string &name) {
    for (const auto &spec : OPTIONS) {
        if (name == spec.name) {
            return &spec;
        }
    }

    return nullptr;
}

// The state of one parsing: the error is set by the first failure.
struct Parser {
    Settings settings{};
    std::string error{};
    bool addressSeen = false;
    bool symbolsSeen = false;

    bool fail(std::string message) {
        if (error.empty()) {
            error = std::move(message);
        }

        return false;
    }

    bool producer() const {
        return settings.mode == Mode::PRODUCER;
    }

    bool parseSymbols(const std::string &flag, const std::string &value) {
        const auto at = value.find('@');
        const auto head = value.substr(0, at);

        if (head.find('{') != std::string::npos) {
            return fail("unsupported QD flag: the symbol length '{len,max}' in '" + flag + " " + value + "'");
        }

        if (head.find('/') != std::string::npos) {
            return fail("unsupported QD flag: the symbol split '/r' or '/s' in '" + flag + " " + value + "'");
        }

        const auto total = parsePositive(head, MAX_SYMBOLS);

        if (!total) {
            return fail(flag + " needs <total> or <total>@<file>, where <total> is a number from 1 to " +
                        std::to_string(MAX_SYMBOLS) + ": '" + value + "'");
        }

        settings.totalSymbols = *total;
        settings.symbolsFile.clear();

        if (at != std::string::npos) {
            settings.symbolsFile = value.substr(at + 1);

            if (settings.symbolsFile.empty()) {
                return fail(flag + " has no file name after '@': '" + value + "'");
            }
        }

        symbolsSeen = true;

        return true;
    }

    bool parseRateOption(const std::string &flag, const std::string &value) {
        if (value.find(';') != std::string::npos) {
            return fail("unsupported QD flag: the rate limiter bucket ';<time>' in '" + flag + " " + value + "'");
        }

        if (value.size() < 3 || value.compare(0, 2, "*/") != 0) {
            return fail(flag + " needs */<rate> (for example */500k): '" + value + "'");
        }

        const auto rate = parseRate(value.substr(2));

        if (!rate) {
            return fail(flag + " has a bad rate (a positive number with an optional k, M or G): '" + value + "'");
        }

        settings.recordsPerSecond = *rate;

        return true;
    }

    bool parseIntegerOption(const std::string &flag, const std::string &value, std::int64_t limit,
                            std::int64_t &out) {
        const auto parsed = parsePositive(value, limit);

        if (!parsed) {
            return fail(flag + " needs a number from 1 to " + std::to_string(limit) + ": '" + value + "'");
        }

        out = *parsed;

        return true;
    }

    bool parseDurationOption(const std::string &flag, const std::string &value, std::int64_t &out) {
        const auto parsed = parseDuration(value);

        if (!parsed) {
            return fail(flag + " needs a positive time with ms, s or m (for example 10s): '" + value + "'");
        }

        out = *parsed;

        return true;
    }

    // [<host>:]<port>, the port from 0 to 65535; the host is the text before the last : and must not be empty.
    bool parseMetricsPort(const std::string &flag, const std::string &value) {
        const auto colon = value.rfind(':');
        const auto host = colon == std::string::npos ? std::string("127.0.0.1") : value.substr(0, colon);
        const auto port = colon == std::string::npos ? value : value.substr(colon + 1);

        if (host.empty() || !isDigits(port) || port.size() > 5 || std::strtol(port.c_str(), nullptr, 10) > 65535) {
            return fail(flag + " needs [<host>:]<port>, where <port> is a number from 0 to 65535: '" + value + "'");
        }

        settings.metricsHost = host;
        settings.metricsPort = static_cast<std::int32_t>(std::strtol(port.c_str(), nullptr, 10));

        return true;
    }

    bool parseCollector(const std::string &flag, const std::string &value) {
        if (value == "stream") {
            settings.collector = Collector::STREAM;
        } else if (value == "ticker") {
            settings.collector = Collector::TICKER;
        } else if (value == "history") {
            return fail(flag + " history is reserved");
        } else {
            return fail(flag + " is stream or ticker: '" + value + "'");
        }

        return true;
    }

    bool parseRole(const std::string &flag, const std::string &value) {
        const auto first = producer() ? Role::PUBLISHER : Role::FEED;
        const auto second = producer() ? Role::STREAM_PUBLISHER : Role::STREAM_FEED;

        if (value == roleName(first)) {
            settings.role = first;
        } else if (value == roleName(second)) {
            settings.role = second;
        } else {
            return fail(flag + " is " + roleName(first) + " or " + roleName(second) + ": '" + value + "'");
        }

        return true;
    }

    bool parseProperty(const std::string &argument) {
        const auto equals = argument.find('=');

        if (equals == std::string::npos || equals < 3) {
            return fail("-D needs <key>=<value>: '" + argument + "'");
        }

        settings.properties.emplace_back(argument.substr(2, equals - 2), argument.substr(equals + 1));

        return true;
    }

    // Applies one option with its value (empty for a switch).
    bool apply(const OptionSpec &spec, const std::string &flag, const std::string &value) {
        switch (spec.option) {
        case Option::SYMBOLS:
            return parseSymbols(flag, value);
        case Option::INSTANCES: {
            std::int64_t instances = 0;

            if (!parseIntegerOption(flag, value, MAX_INSTANCES, instances)) {
                return false;
            }

            settings.instances = static_cast<std::int32_t>(instances);

            return true;
        }
        case Option::RATE:
            return parseRateOption(flag, value);
        case Option::STAT:
            return parseDurationOption(flag, value, settings.periodMillis);
        case Option::NAME:
            if (value.empty()) {
                return fail(flag + " needs a non-empty name");
            }

            settings.name = value;

            return true;
        case Option::COLLECTOR:
            return parseCollector(flag, value);
        case Option::EVENT:
            if (value == "Trade") {
                settings.event = EventKind::TRADE;
            } else if (value == "TimeAndSale") {
                settings.event = EventKind::TIME_AND_SALE;
            } else {
                return fail(flag + " is Trade or TimeAndSale: '" + value + "'");
            }

            return true;
        case Option::BATCH: {
            std::int64_t batch = 0;

            if (!parseIntegerOption(flag, value, MAX_BATCH, batch)) {
                return false;
            }

            settings.batch = static_cast<std::int32_t>(batch);

            return true;
        }
        case Option::REUSE:
            settings.reuse = true;

            return true;
        case Option::ROLE:
            return parseRole(flag, value);
        case Option::DURATION: {
            std::int64_t duration = 0;

            if (!parseDurationOption(flag, value, duration)) {
                return false;
            }

            settings.durationMillis = duration;

            return true;
        }
        case Option::WARMUP: {
            std::int64_t warmup = 0;

            if (!parseDurationOption(flag, value, warmup)) {
                return false;
            }

            settings.warmupMillis = warmup;

            return true;
        }
        case Option::SUMMARY:
            if (value.empty() || !isWritableDirectoryOf(value)) {
                return fail(flag + " cannot write to the directory of '" + value + "'");
            }

            settings.summaryFile = value;

            return true;
        case Option::METRICS_PORT:
            return parseMetricsPort(flag, value);
        case Option::LEGACY:
            // The API has a single way to publish, so there is no deprecated call to choose.
            return fail("unsupported option: " + flag);
        case Option::HELP:
            settings.help = true;

            return true;
        case Option::WILDCARD:
            return fail("unsupported QD flag: " + flag + " (the wildcard subscription)");
        }

        return fail("unknown flag " + flag);
    }

    // The rules between the options, after all of them are read (a repeated option takes its last value).
    bool validate() {
        if (!addressSeen || settings.address.empty()) {
            return fail("the address is missing");
        }

        if (!symbolsSeen) {
            return fail("-S is missing");
        }

        if (settings.instances > 1 && isServerAddress(settings.address)) {
            return fail("-C " + std::to_string(settings.instances) + " cannot be used with the server address '" +
                        settings.address + "': every instance would bind the same port");
        }

        const auto event = eventName(settings.event);

        if (settings.collector == Collector::TICKER && settings.event != EventKind::TRADE) {
            return fail("-c ticker needs --event Trade: " + event + " has no ticker contract");
        }

        // Without --role, -c selects a role that receives its contract; without -c, either role of the mode is valid.
        if (!settings.collector.has_value() || !settings.role.has_value()) {
            return true;
        }

        const auto role = *settings.role;

        if (settings.collector == Collector::TICKER) {
            // The ticker contract needs the roles that write and read it.
            if (role == Role::STREAM_PUBLISHER || role == Role::STREAM_FEED) {
                return fail("--role " + roleName(role) + " conflicts with -c ticker for " + event);
            }
        } else if (role == Role::FEED && settings.event == EventKind::TRADE) {
            // FEED receives Trade through the ticker contract, not the stream contract of -c stream.
            return fail("--role " + roleName(role) + " conflicts with -c stream for " + event);
        }

        return true;
    }

    bool run(const std::vector<std::string> &args) {
        if (args.empty()) {
            return fail("no mode");
        }

        if (args[0] == "--help") {
            settings.help = true;

            return true;
        }

        if (args[0] == "p") {
            settings.mode = Mode::PRODUCER;
        } else if (args[0] == "c") {
            settings.mode = Mode::CONSUMER;
        } else {
            return fail("unknown mode '" + args[0] + "'; the mode is p or c");
        }

        settings.arguments = args;

        for (std::size_t index = 1; index < args.size(); ++index) {
            const auto &argument = args[index];

            if (argument.compare(0, 2, "-D") == 0) {
                if (!parseProperty(argument)) {
                    return false;
                }

                continue;
            }

            // The options of the native runtime (-Xmx, -XX:...) have no meaning here: the API starts its isolate itself.
            if (argument.compare(0, 2, "-X") == 0) {
                return fail("unsupported option: " + argument);
            }

            if (argument.size() < 2 || argument[0] != '-') {
                if (addressSeen) {
                    return fail("unexpected argument '" + argument + "' after the address '" + settings.address + "'");
                }

                settings.address = argument;
                addressSeen = true;

                continue;
            }

            // A long option may carry its value after `=`.
            auto flag = argument;
            std::optional<std::string> inlineValue{};

            if (argument.compare(0, 2, "--") == 0) {
                if (const auto equals = argument.find('='); equals != std::string::npos) {
                    flag = argument.substr(0, equals);
                    inlineValue = argument.substr(equals + 1);
                }
            }

            const auto *spec = findOption(flag);

            if (spec == nullptr) {
                return fail("unknown flag " + flag);
            }

            if (spec->producerOnly && !producer()) {
                return fail(flag + " is an option of the mode p");
            }

            std::string value{};

            if (spec->hasValue) {
                if (inlineValue.has_value()) {
                    value = *inlineValue;
                } else if (index + 1 < args.size()) {
                    value = args[++index];
                } else {
                    return fail("missing value of " + flag);
                }
            } else if (inlineValue.has_value()) {
                return fail(flag + " takes no value: '" + argument + "'");
            }

            if (!apply(*spec, flag, value)) {
                return false;
            }

            // --help ends the parsing: the usage is printed whatever follows.
            if (settings.help) {
                return true;
            }
        }

        return validate();
    }
};

} // namespace

std::optional<std::int64_t> parseDuration(const std::string &text) {
    std::int64_t multiplier = 0;
    std::size_t suffixLength = 0;

    if (text.size() > 2 && text.compare(text.size() - 2, 2, "ms") == 0) {
        multiplier = 1;
        suffixLength = 2;
    } else if (text.size() > 1 && text.back() == 's') {
        multiplier = 1'000;
        suffixLength = 1;
    } else if (text.size() > 1 && text.back() == 'm') {
        multiplier = 60'000;
        suffixLength = 1;
    } else {
        return std::nullopt;
    }

    const auto number = text.substr(0, text.size() - suffixLength);

    if (!isDigits(number) || number.size() > 15) {
        return std::nullopt;
    }

    const auto value = std::strtoll(number.c_str(), nullptr, 10);

    if (value < 1 || value > std::numeric_limits<std::int64_t>::max() / multiplier) {
        return std::nullopt;
    }

    return value * multiplier;
}

std::optional<std::int64_t> parseRate(const std::string &text) {
    if (text.empty()) {
        return std::nullopt;
    }

    double multiplier = 1;
    auto number = text;

    switch (text.back()) {
    case 'k':
        multiplier = 1e3;
        number.pop_back();
        break;
    case 'M':
        multiplier = 1e6;
        number.pop_back();
        break;
    case 'G':
        multiplier = 1e9;
        number.pop_back();
        break;
    default:
        break;
    }

    // Digits with at most one dot: no sign, no exponent, no spaces.
    if (number.empty() || number.front() == '.' || number.back() == '.') {
        return std::nullopt;
    }

    std::size_t dots = 0;

    for (const char symbol : number) {
        if (symbol == '.') {
            ++dots;
        } else if (symbol < '0' || symbol > '9') {
            return std::nullopt;
        }
    }

    if (dots > 1) {
        return std::nullopt;
    }

    const auto rate = std::strtod(number.c_str(), nullptr) * multiplier;

    if (!(rate >= 1.0) || rate > 1e12) {
        return std::nullopt;
    }

    return static_cast<std::int64_t>(std::llround(rate));
}

bool isWritableDirectoryOf(const std::string &file) {
    namespace fs = std::filesystem;

    std::error_code error;
    const fs::path path(file);
    const auto directory = path.has_parent_path() ? path.parent_path() : fs::path(".");

    if (!fs::is_directory(directory, error) || fs::is_directory(path, error)) {
        return false;
    }

    // A probe file next to the summary: the only check of the permissions that works alike everywhere.
    const auto probe = fs::path(path).concat(".probe");
    const bool existed = fs::exists(probe, error);
    bool writable = false;

    {
        std::ofstream stream(probe, std::ios::binary | std::ios::app);

        writable = stream.good();
    }

    if (writable && !existed) {
        fs::remove(probe, error);
    }

    return writable;
}

bool isServerAddress(const std::string &address) {
    return !address.empty() && address.front() == ':';
}

Role endpointRole(const Settings &settings) {
    if (settings.role.has_value()) {
        return *settings.role;
    }

    if (settings.mode == Mode::PRODUCER) {
        return settings.collector == Collector::STREAM ? Role::STREAM_PUBLISHER : Role::PUBLISHER;
    }

    return settings.collector == Collector::TICKER ? Role::FEED : Role::STREAM_FEED;
}

std::string roleName(Role role) {
    switch (role) {
    case Role::FEED:
        return "FEED";
    case Role::STREAM_FEED:
        return "STREAM_FEED";
    case Role::PUBLISHER:
        return "PUBLISHER";
    case Role::STREAM_PUBLISHER:
        return "STREAM_PUBLISHER";
    }

    return "UNKNOWN";
}

std::string collectorName(Collector collector) {
    return collector == Collector::STREAM ? "stream" : "ticker";
}

std::string eventName(EventKind event) {
    return event == EventKind::TRADE ? "Trade" : "TimeAndSale";
}

bool isConflated(const Settings &settings) {
    return settings.mode == Mode::CONSUMER && endpointRole(settings) == Role::FEED &&
           settings.event == EventKind::TRADE;
}

ParsedSettings parseArguments(const std::vector<std::string> &args) {
    Parser parser;
    ParsedSettings result;

    result.ok = parser.run(args);
    result.error = parser.error;

    if (result.ok) {
        result.settings = std::move(parser.settings);
    }

    return result;
}

std::string usageText() {
    return std::string(
               "Usage: NetTest p|c <address> -S <symbols> [options]\n"
               "\n"
               "  p                    publish generated events\n"
               "  c                    receive them and report the end-to-end lag\n"
               "  <address>            host:port to connect to, :port to listen on\n"
               "  -S <total>           the symbols: the <total> generated symbols of QD NetTest\n"
               "  -S <total>@<file>    the first <total> symbols of the IPF file, in file order\n"
               "  -C <n>               the number of instances, default 1 (not above 1 with a :port address);\n"
               "                       on c the instances share the symbols\n"
               "  -r, --rate-limit */<rate>\n"
               "                       p: the total rate in records per second, with k, M or G (*/1M)\n"
               "  -s, --stat <period>  the statistics period, default 10s (ms, s or m)\n"
               "  -n, --name <name>    the name in the statistics lines, default NetTest\n"
               "  -c, --collector <contract>\n"
               "                       stream or ticker: the contract that the consumer receives and the\n"
               "                       default roles (none: PUBLISHER and STREAM_FEED)\n"
               "  --event <type>       Trade (default) or TimeAndSale\n"
               "  --role <role>        PUBLISHER or STREAM_PUBLISHER on p, FEED or STREAM_FEED on c\n"
               "  --batch <n>          p: the events per publishing call, default 100\n"
               "  --reuse              p: republish the pre-built events in place instead of copying them\n"
               "  --duration <time>    end the run after the time (ms, s or m); Ctrl+C ends it at once\n"
               "  --warmup <time>      leave the first <time> out of the e2e lag of the summary; on c it starts\n"
               "                       at the first record, on p at the first batch\n"
               "  --summary <file>     write the JSON summary of the run (nettest-summary/1) at the end\n"
               "  --metrics-port [<host>:]<port>\n"
               "                       serve /metrics in the Prometheus text format; the host defaults to\n"
               "                       127.0.0.1, port 0 takes a free port\n"
               "  -D<key>=<value>      set a system property before the first endpoint is created\n"
               "                       (dxscheme.nanoTime also goes to the builder of every endpoint, default "
               "true)\n"
               "  --help               print this text and exit\n"
               "\n"
               "A long option also takes its value as --option=value.\n"
               "\n"
               "spec_version ") +
           SPEC_VERSION + "\n";
}

std::string endpointNanoTime(const Settings &settings) {
    std::string value = "true";

    for (const auto &[key, given] : settings.properties) {
        if (key == NANO_TIME_PROPERTY) {
            value = given;
        }
    }

    return value;
}

bool isNanoTimeOn(const Settings &settings) {
    const auto value = endpointNanoTime(settings);

    if (value.size() != 4) {
        return false;
    }

    for (std::size_t index = 0; index < 4; ++index) {
        if (std::tolower(static_cast<unsigned char>(value[index])) != "true"[index]) {
            return false;
        }
    }

    return true;
}

// The options that the parser takes but whose behavior comes with the next stages of the tool: the consumer mode, the
// summary file, the warm-up and the /metrics port. One place, so that every stage narrows it.
std::optional<std::string> unsupportedOption(const Settings &settings) {
    if (settings.mode == Mode::CONSUMER) {
        return "unsupported option: c";
    }

    if (!settings.summaryFile.empty()) {
        return "unsupported option: --summary";
    }

    if (settings.warmupMillis.has_value()) {
        return "unsupported option: --warmup";
    }

    if (settings.metricsPort.has_value()) {
        return "unsupported option: --metrics-port";
    }

    return std::nullopt;
}

} // namespace dxfcpp::tools::nettest
