// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "TapeSupport.hpp"

#include <doctest.h>

#include <algorithm>
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
#include <typeinfo>

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

// "20231114-221320.123+0300" -> "1700000000123" (UTC milliseconds), the offset ("+0300") goes to `offset`; other
// values are returned as they are.
std::string normalizeTime(const std::string &value, std::string &offset) {
    static const std::regex timeRegex(
        R"(^(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2})(?:\.(\d{3}))?([+-])(\d{2})(\d{2})$)");
    std::smatch m;

    if (!std::regex_match(value, m, timeRegex)) {
        return value;
    }

    offset = m[8].str() + m[9].str() + m[10].str();

    const auto days =
        daysFromCivil(std::stoll(m[1]), static_cast<unsigned>(std::stoi(m[2])), static_cast<unsigned>(std::stoi(m[3])));
    const auto millis = m[7].matched ? std::stoll(m[7]) : 0;
    const auto offsetMinutes = (std::stoll(m[9]) * 60 + std::stoll(m[10])) * (m[8] == "+" ? 1 : -1);
    const auto utcMillis = ((days * 24 + std::stoll(m[4])) * 60 + std::stoll(m[5]) - offsetMinutes) * 60'000 +
                           std::stoll(m[6]) * 1000 + millis;

    return std::to_string(utcMillis);
}

// QD writes the UTC offset of a time in whole minutes. Before a time zone switched to standard time, its offset was
// the local mean time with seconds (Europe/Moscow: +02:30:17 until 1919), so such a time written in that zone is parsed
// up to 59 seconds off. The golden tapes are written in UTC, the tapes of the tests in the zone of the process (which
// cannot be changed for the native SDK), so these times are compared with that tolerance.
bool isSameTimeInOtherZone(const TapeRecord &expected, const TapeRecord &actual, const std::string &column) {
    const auto expectedOffset = expected.timeOffsets.find(column);
    const auto actualOffset = actual.timeOffsets.find(column);

    if (expectedOffset == expected.timeOffsets.end() || actualOffset == actual.timeOffsets.end() ||
        expectedOffset->second == actualOffset->second) {
        return false;
    }

    const auto expectedMillis = std::stoll(expected.fields.at(column));
    const auto actualMillis = std::stoll(actual.fields.at(column));
    const auto difference = actualMillis - expectedMillis;

    return expectedMillis < 0 && difference % 1000 == 0 && difference > -60'000 && difference < 60'000;
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

    try {
        endpoint->getPublisher()->publishEvents(events.begin(), events.end());
    } catch (...) {
        endpoint->closeAndAwaitTermination();

        throw;
    }

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
            std::string offset{};

            record.fields[columns->second[i]] = i + 1 < values.size() ? normalizeTime(values[i + 1], offset) : "";

            if (!offset.empty()) {
                record.timeOffsets[columns->second[i]] = offset;
            }
        }

        // Named values after the columns, e.g. `EventFlags=TX_PENDING,SNAPSHOT_END`.
        for (std::size_t i = columns->second.size() + 1; i < values.size(); i++) {
            const auto separator = values[i].find('=');

            if (separator == std::string::npos) {
                throw std::runtime_error("parseTextTape: unexpected value " + values[i] + " after the columns of " +
                                         record.name);
            }

            record.fields[values[i].substr(0, separator)] = values[i].substr(separator + 1);
        }

        records.push_back(std::move(record));
    }

    return records;
}

std::vector<std::string> compareTapes(const std::string &expectedPath, const std::string &actualPath) {
    const auto keyOf = [](const TapeRecord &record) {
        const auto symbol = record.fields.find("EventSymbol");

        return record.name + "@" + (symbol == record.fields.end() ? "" : symbol->second);
    };

    std::map<std::string, TapeRecord> actual{};

    for (auto &record : parseTextTape(actualPath)) {
        const auto key = keyOf(record);

        if (!actual.emplace(key, std::move(record)).second) {
            throw std::runtime_error("compareTapes: two records " + key + " in " + actualPath);
        }
    }

    std::vector<std::string> differences{};

    for (const auto &expected : parseTextTape(expectedPath)) {
        const auto key = keyOf(expected);
        const auto symbol = key.substr(expected.name.size());
        const auto found = actual.find(key);

        if (found == actual.end()) {
            differences.push_back(key + ": missing");

            continue;
        }

        auto actualFields = found->second.fields;

        for (const auto &[column, value] : expected.fields) {
            const auto actualValue = actualFields.find(column);
            const auto actualText = actualValue == actualFields.end() ? "<missing>" : actualValue->second;

            if (column != "EventTime" && actualText != value &&
                !isSameTimeInOtherZone(expected, found->second, column)) {
                differences.push_back(expected.name + "." + column + symbol + ": expected=" + value +
                                      ", actual=" + actualText);
            }

            if (actualValue != actualFields.end()) {
                actualFields.erase(actualValue);
            }
        }

        for (const auto &[column, value] : actualFields) {
            differences.push_back(expected.name + "." + column + symbol + ": expected=<missing>, actual=" + value);
        }

        actual.erase(found);
    }

    for (const auto &[key, record] : actual) {
        differences.push_back(key + ": unexpected");
    }

    return differences;
}

void checkEvents(const std::vector<std::shared_ptr<EventType>> &expected,
                 const std::vector<std::shared_ptr<EventType>> &actual) {
    const auto keyOf = [](const std::shared_ptr<EventType> &event) {
        std::string key = typeid(*event).name();

        if (const auto order = event->sharedAs<OrderBase>()) {
            key += "#" + order->getSource().name();
        }

        if (const auto candle = event->sharedAs<Candle>()) {
            return key + "@" + candle->getEventSymbol().toString();
        }

        if (const auto withSymbol = event->sharedAs<EventTypeWithSymbol<std::string>>()) {
            return key + "@" + withSymbol->getEventSymbol();
        }

        return key;
    };

    std::map<std::string, std::shared_ptr<EventType>> actualByKey{};

    for (const auto &event : actual) {
        actualByKey[keyOf(event)] = event;
    }

    for (const auto &event : expected) {
        const auto key = keyOf(event);

        CAPTURE(key);

        const auto found = actualByKey.find(key);

        REQUIRE(found != actualByKey.end());
        CHECK(found->second->toString() == event->toString());
    }
}

bool isKnownDifference(const std::string &difference, const std::vector<std::string> &known) {
    return std::any_of(known.begin(), known.end(), [&](const auto &prefix) {
        return difference.rfind(prefix + "@", 0) == 0 || difference.rfind(prefix + ":", 0) == 0;
    });
}

void checkDifferences(const std::vector<std::string> &differences, const std::vector<std::string> &known,
                      bool withKnown) {
    std::string selected{};

    for (const auto &difference : differences) {
        if (withKnown || !isKnownDifference(difference, known)) {
            selected += "\n  " + difference;
        }
    }

    INFO("differences:", selected);
    CHECK(selected.empty());
}

} // namespace dxfcpp::test
