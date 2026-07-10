// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../isolated/metrics/IsolatedMetrics.hpp"
#include "../system/System.hpp"
#include "./Common.hpp"
#include "./NonCopyable.hpp"
#include "./utils/StringUtils.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT MetricsManager : private NonCopyable<MetricsManager> {
    static constexpr auto METRICS_GROUPS_PROPERTY_NAME = "MetricsManager.Dump.MetricsGroups";

    template <typename T> struct Stats {
        std::uint64_t count{1};
        T value{};
        T minValue{std::numeric_limits<T>::max()};
        T maxValue{std::numeric_limits<T>::min()};
        T avgValue{};
    };

    using Value = std::variant<std::string, Stats<double>, Stats<std::int64_t>>;

    private:
    std::mutex mtx_{};
    std::unordered_map<std::string, Value> data_{};
    std::optional<std::unordered_set<std::string>> groupsToDump_{std::nullopt};

    void setImpl(const std::string &name, const std::string &value);

    template <typename T> void recalculateStatsImpl(Stats<T> &stats, T value) {
        stats.value = value;
        stats.minValue = std::min(stats.minValue, value);
        stats.maxValue = std::max(stats.maxValue, value);
        stats.avgValue = static_cast<T>((static_cast<double>(stats.avgValue) * static_cast<double>(stats.count) +
                                         static_cast<double>(stats.value)) /
                                        (static_cast<double>(stats.count) + 1.0));
        ++stats.count;
    }

    void setImpl(const std::string &name, double value);

    template <Integral T> void setImpl(const std::string &name, T value) {
        if (!data_.contains(name)) {
            data_[name] = Value{Stats<std::int64_t>{1, value, value, value, value}};

            return;
        }

        if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
            recalculateStatsImpl(std::get<Stats<std::int64_t>>(data_[name]), value);
        }
    }

    Stats<std::int64_t> getAsI64Impl(const std::string &name);

    Stats<double> getAsDoubleImpl(const std::string &name);

    public:
    template <typename T>
    static std::string toString(const std::string &key, const Stats<T> &stats, bool compact = false) {
        std::stringstream ss{};

        if (compact) {
            ss << key + ": {Count = " << stats.count << ", Value = " << stats.value << ", Min = " << stats.minValue
               << ", Max = " << stats.maxValue << ", Avg = " << stats.avgValue << '}';
        } else {
            ss << key + ".Count: " << stats.count << '\n';
            ss << key + ".Value: " << stats.value << '\n';
            ss << key + ".Min: " << stats.minValue << '\n';
            ss << key + ".Max: " << stats.maxValue << '\n';
            ss << key + ".Avg: " << stats.avgValue << '\n';
        }

        return ss.str();
    }

    static std::string toString(const std::string &key, const Value &value, bool compact);

    std::optional<Value> get(const std::string &name);

    std::string getAsString(const std::string &name);

    Stats<double> getAsDouble(const std::string &name);

    Stats<std::int64_t> getAsI64(const std::string &name);

    template <typename T> void set(const std::string &name, const T &value) {
        std::lock_guard lockGuard{mtx_};

        setImpl(name, value);
    }

    void inc(const std::string &name);

    template <Integral T> void add(const std::string &name, T value) {
        std::lock_guard lockGuard{mtx_};

        setImpl(name, getAsI64Impl(name).value + value);
    }

    void add(const std::string &name, double value);

    std::string dump();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()