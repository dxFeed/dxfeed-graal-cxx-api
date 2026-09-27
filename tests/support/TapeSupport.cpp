// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "TapeSupport.hpp"

#include <atomic>
#include <cstdio>
#include <filesystem>

#ifdef _WIN32
#    include <process.h>
#    define DXFCXX_TEST_GETPID _getpid
#else
#    include <unistd.h>
#    define DXFCXX_TEST_GETPID getpid
#endif
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifndef DXFCXX_TEST_DATA_DIR
#    error "DXFCXX_TEST_DATA_DIR must point to tests/data"
#endif

namespace dxfcpp::test {

namespace {

std::shared_ptr<DXEndpoint> buildEndpoint(DXEndpoint::Role role) {
    return DXEndpoint::newBuilder()
        ->withRole(role)
        ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
        ->withProperty(DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true")
        ->build();
}

bool isDerivedOrder(const std::shared_ptr<EventType> &event) {
    const auto order = event->sharedAs<OrderBase>();

    return order && OrderSource::isSpecialSourceId(order->getSource().id());
}

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
std::int64_t daysFromCivil(std::int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const auto yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

// "20231114-221320.123+0300" -> "1700000000123" (UTC milliseconds); other values are returned as they are.
std::string normalizeTime(const std::string &value) {
    static const std::regex timeRegex(R"(^(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2})(?:\.(\d{3}))?([+-])(\d{2})(\d{2})$)");
    std::smatch m;

    if (!std::regex_match(value, m, timeRegex)) {
        return value;
    }

    const auto days = daysFromCivil(std::stoll(m[1]), static_cast<unsigned>(std::stoi(m[2])),
                                    static_cast<unsigned>(std::stoi(m[3])));
    const auto millis = m[7].matched ? std::stoll(m[7]) : 0;
    const auto offsetMinutes = (std::stoll(m[9]) * 60 + std::stoll(m[10])) * (m[8] == "+" ? 1 : -1);
    const auto utcMillis =
        ((days * 24 + std::stoll(m[4])) * 60 + std::stoll(m[5]) - offsetMinutes) * 60'000 + std::stoll(m[6]) * 1000 +
        millis;

    return std::to_string(utcMillis);
}

std::vector<std::string> splitTabs(const std::string &line) {
    std::vector<std::string> result{};
    std::string current{};
    std::istringstream stream(line);

    while (std::getline(stream, current, '\t')) {
        result.push_back(current);
    }

    if (!line.empty() && line.back() == '\t') {
        result.emplace_back();
    }

    return result;
}

} // namespace

std::string testDataPath(const std::string &relativePath) {
    return std::string(DXFCXX_TEST_DATA_DIR) + "/" + relativePath;
}

std::string temporaryFilePath(const std::string &name) {
    static std::atomic<int> counter{};

    // The process id keeps the names unique when several test processes run at once (ctest -j).
    return std::string(DXFCXX_TEST_TEMP_DIR) + "/" + std::to_string(DXFCXX_TEST_GETPID()) + "-" +
           std::to_string(counter++) + "-" + name;
}

std::vector<std::shared_ptr<EventType>> readTape(const std::string &path,
                                                 const std::vector<std::string> &candleSymbols) {
    const auto endpoint = buildEndpoint(DXEndpoint::Role::STREAM_FEED);
    std::mutex mutex{};
    std::vector<std::shared_ptr<EventType>> events{};
    const auto listener = [&](const std::vector<std::shared_ptr<EventType>> &received) {
        std::lock_guard lock{mutex};

        for (const auto &event : received) {
            if (!isDerivedOrder(event)) {
                events.push_back(event);
            }
        }
    };

    std::vector<EventTypeEnum> types{};

    for (const auto &type : EventTypeEnum::ALL) {
        if (type.get() != Candle::TYPE) {
            types.push_back(type.get());
        }
    }

    const auto subscription = endpoint->getFeed()->createSubscription(types.begin(), types.end());

    subscription->addEventListener(listener);
    subscription->addSymbols(WildcardSymbol::ALL);

    const auto candleSubscription = endpoint->getFeed()->createSubscription(Candle::TYPE);

    candleSubscription->addEventListener(listener);

    for (const auto &symbol : candleSymbols) {
        candleSubscription->addSymbols(CandleSymbol::valueOf(symbol));
    }

    endpoint->connect("file:" + path + "[speed=max]");
    endpoint->awaitNotConnected();
    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();

    std::lock_guard lock{mutex};

    return events;
}

// QD parses `tape:C:/...` as `host:port`, so on Windows the tape connector gets a path relative to the current
// directory (`file:` accepts absolute Windows paths).
static std::string tapeConnectorPath(const std::string &path) {
#ifdef _WIN32
    const auto relative = std::filesystem::relative(path).generic_string();

    if (relative.empty() || relative.find(':') != std::string::npos) {
        throw std::runtime_error("writeTextTape: " + path + " is not reachable by a relative path from " +
                                 std::filesystem::current_path().string());
    }

    return relative;
#else
    return path;
#endif
}

void writeTextTape(const std::string &path, const std::vector<std::shared_ptr<EventType>> &events) {
    std::remove(path.c_str());

    const auto endpoint = buildEndpoint(DXEndpoint::Role::PUBLISHER);

    endpoint->connect("tape:" + tapeConnectorPath(path) + "[format=text]");
    endpoint->getPublisher()->publishEvents(events.begin(), events.end());
    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();
}

std::vector<TapeRecord> parseTextTape(const std::string &path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("parseTextTape: cannot open " + path);
    }

    std::map<std::string, std::vector<std::string>> columnsByRecord{};
    std::vector<TapeRecord> records{};
    std::string line{};

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty() || line.rfind("==", 0) == 0) {
            continue;
        }

        auto values = splitTabs(line);

        if (line[0] == '=') {
            const auto name = values.front().substr(1);

            values.erase(values.begin());
            columnsByRecord[name] = values;

            continue;
        }

        const auto columns = columnsByRecord.find(values.front());

        if (columns == columnsByRecord.end()) {
            throw std::runtime_error("parseTextTape: no columns for the record " + values.front());
        }

        TapeRecord record{values.front(), {}};

        for (std::size_t i = 0; i < columns->second.size(); i++) {
            record.fields[columns->second[i]] = i + 1 < values.size() ? normalizeTime(values[i + 1]) : "";
        }

        records.push_back(std::move(record));
    }

    return records;
}

} // namespace dxfcpp::test
