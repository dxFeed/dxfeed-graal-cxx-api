// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./Common.hpp"
#include "./NonCopyable.hpp"
#include "./utils/StringUtils.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

struct MetricsManager : private NonCopyable<MetricsManager> {
    template <typename T> struct Stats {
        std::uint64_t count{1};
        T value{};
        T minValue{std::numeric_limits<T>::max()};
        T maxValue{std::numeric_limits<T>::min()};
    };

    using Value = std::variant<std::string, Stats<double>, Stats<std::int64_t>>;

    private:
    std::mutex mtx_{};
    std::unordered_map<std::string, Value> data_{};

    void setImpl(const std::string &name, const std::string &value) {
        data_[name] = value;
    }

    template <typename T> void recalculateStatsImpl(Stats<T> &stats, T value) {
        ++stats.count;
        stats.value = value;
        stats.minValue = std::min(stats.minValue, value);
        stats.maxValue = std::max(stats.maxValue, value);
    }

    void setImpl(const std::string &name, double value) {
        if (!data_.contains(name)) {
            data_[name] = Value{Stats{1, value, value, value}};

            return;
        }

        if (std::holds_alternative<Stats<double>>(data_.at(name))) {
            recalculateStatsImpl(std::get<Stats<double>>(data_[name]), value);
        }
    }

    template <Integral T> void setImpl(const std::string &name, T value) {
        if (!data_.contains(name)) {
            data_[name] = Value{Stats<std::int64_t>{1, value, value, value}};

            return;
        }

        if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
            recalculateStatsImpl(std::get<Stats<std::int64_t>>(data_[name]), value);
        }
    }

    Stats<std::int64_t> getAsI64Impl(const std::string &name) {
        if (!data_.contains(name)) {
            return {};
        }

        if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
            return std::get<Stats<std::int64_t>>(data_.at(name));
        }

        if (std::holds_alternative<Stats<double>>(data_.at(name))) {
            auto &[count, value, minValue, maxValue] = std::get<Stats<double>>(data_.at(name));

            return Stats{count, static_cast<std::int64_t>(value), static_cast<std::int64_t>(minValue),
                         static_cast<std::int64_t>(maxValue)};
        }

        if (std::holds_alternative<std::string>(data_.at(name))) {
            const auto &value = std::get<std::string>(data_.at(name));

            return Stats<std::int64_t>{1, std::stoll(value), std::stoll(value), std::stoll(value)};
        }

        return {};
    }

    Stats<double> getAsDoubleImpl(const std::string &name) {
        if (!data_.contains(name)) {
            return {};
        }

        if (std::holds_alternative<Stats<double>>(data_.at(name))) {
            return std::get<Stats<double>>(data_.at(name));
        }

        if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
            auto &[count, value, minValue, maxValue] = std::get<Stats<std::int64_t>>(data_.at(name));

            return Stats{count, static_cast<double>(value), static_cast<double>(minValue),
                         static_cast<double>(maxValue)};
        }

        if (std::holds_alternative<std::string>(data_.at(name))) {
            const auto &value = std::get<std::string>(data_.at(name));

            return Stats{1, std::stod(value), std::stod(value), std::stod(value)};
        }

        return {};
    }

    public:
    template <typename T>
    static std::string toString(const std::string &key, const Stats<T> &stats, bool compact = false) {
        std::stringstream ss{};

        if (compact) {
            ss << key + ": {Count = " << stats.count << ", Value = " << stats.value << ", Min = " << stats.minValue
               << ", Max = " << stats.maxValue << '}';
        } else {
            ss << key + ".Count: " << stats.count << '\n';
            ss << key + ".Value: " << stats.value << '\n';
            ss << key + ".Min: " << stats.minValue << '\n';
            ss << key + ".Max: " << stats.maxValue << '\n';
        }

        return ss.str();
    }

    static std::string toString(const std::string &key, const Value &value, bool compact) {
        return std::visit(Overloads{
                              [key, compact](const std::string &s) {
                                  if (compact) {
                                      return s;
                                  }

                                  return key + ": " + s + '\n';
                              },
                              [key, compact](const auto &n) {
                                  return toString(key, n, compact);
                              },
                          },
                          value);
    }

    std::optional<Value> get(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (!data_.contains(name)) {
            return std::nullopt;
        }

        return {data_.at(name)};
    }

    std::string getAsString(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (!data_.contains(name)) {
            return String::EMPTY;
        }

        return toString(name, data_.at(name), true);
    }

    Stats<double> getAsDouble(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        return getAsDoubleImpl(name);
    }

    Stats<std::int64_t> getAsI64(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        return getAsI64Impl(name);
    }

    template <typename T> void set(const std::string &name, const T &value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        setImpl(name, value);
    }

    void inc(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        setImpl(name, getAsI64Impl(name).value + 1);
    }

    template <Integral T> void add(const std::string &name, T value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        setImpl(name, getAsI64Impl(name).value + value);
    }

    void add(const std::string &name, double value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        setImpl(name, getAsDoubleImpl(name).value + value);
    }

    std::string dump() {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (data_.empty()) {
            return String::EMPTY;
        }

        std::map<std::string, Value> records{data_.begin(), data_.end()};
        std::string result{};

        for (const auto &[key, value] : records) {
            result += toString(key, value, false);
        }

        return result.substr(0, result.size() - 1);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()