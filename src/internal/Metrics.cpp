// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Metrics.hpp"

DXFCPP_BEGIN_NAMESPACE

void MetricsManager::setImpl(const std::string &name, const std::string &value) {
    data_[name] = value;
}

void MetricsManager::setImpl(const std::string &name, double value) {
    if (!data_.contains(name)) {
        data_[name] = Value{Stats{1, value, value, value, value}};

        return;
    }

    if (std::holds_alternative<Stats<double>>(data_.at(name))) {
        recalculateStatsImpl(std::get<Stats<double>>(data_[name]), value);
    }
}

MetricsManager::Stats<long long> MetricsManager::getAsI64Impl(const std::string &name) {
    if (!data_.contains(name)) {
        return {};
    }

    if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
        return std::get<Stats<std::int64_t>>(data_.at(name));
    }

    if (std::holds_alternative<Stats<double>>(data_.at(name))) {
        auto &[count, value, minValue, maxValue, avgValue] = std::get<Stats<double>>(data_.at(name));

        return Stats{count, static_cast<std::int64_t>(value), static_cast<std::int64_t>(minValue),
                     static_cast<std::int64_t>(maxValue), static_cast<std::int64_t>(avgValue)};
    }

    if (std::holds_alternative<std::string>(data_.at(name))) {
        const auto &value = std::get<std::string>(data_.at(name));

        return Stats<std::int64_t>{1, std::stoll(value), std::stoll(value), std::stoll(value)};
    }

    return {};
}

MetricsManager::Stats<double> MetricsManager::getAsDoubleImpl(const std::string &name) {
    if (!data_.contains(name)) {
        return {};
    }

    if (std::holds_alternative<Stats<double>>(data_.at(name))) {
        return std::get<Stats<double>>(data_.at(name));
    }

    if (std::holds_alternative<Stats<std::int64_t>>(data_.at(name))) {
        auto &[count, value, minValue, maxValue, avgValue] = std::get<Stats<std::int64_t>>(data_.at(name));

        return Stats{count, static_cast<double>(value), static_cast<double>(minValue), static_cast<double>(maxValue),
                     static_cast<double>(avgValue)};
    }

    if (std::holds_alternative<std::string>(data_.at(name))) {
        const auto &value = std::get<std::string>(data_.at(name));

        return Stats{1, std::stod(value), std::stod(value), std::stod(value)};
    }

    return {};
}

std::string MetricsManager::toString(const std::string &key, const Value &value, bool compact) {
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

std::optional<MetricsManager::Value> MetricsManager::get(const std::string &name) {
    std::lock_guard lockGuard{mtx_};

    if (!data_.contains(name)) {
        return std::nullopt;
    }

    return {data_.at(name)};
}

std::string MetricsManager::getAsString(const std::string &name) {
    std::lock_guard lockGuard{mtx_};

    if (!data_.contains(name)) {
        return String::EMPTY;
    }

    return toString(name, data_.at(name), true);
}

MetricsManager::Stats<double> MetricsManager::getAsDouble(const std::string &name) {
    std::lock_guard lockGuard{mtx_};

    return getAsDoubleImpl(name);
}

MetricsManager::Stats<long long> MetricsManager::getAsI64(const std::string &name) {
    std::lock_guard lockGuard{mtx_};

    return getAsI64Impl(name);
}

void MetricsManager::inc(const std::string &name) {
    std::lock_guard lockGuard{mtx_};

    setImpl(name, getAsI64Impl(name).value + 1);
}

void MetricsManager::add(const std::string &name, double value) {
    std::lock_guard lockGuard{mtx_};

    setImpl(name, getAsDoubleImpl(name).value + value);
}

std::string MetricsManager::dump() {
    std::lock_guard lockGuard{mtx_};

    if (!groupsToDump_) {
        const auto groupsString = System::getProperty(METRICS_GROUPS_PROPERTY_NAME);
        std::unordered_set<std::string> groups{};

        if (!groupsString.empty()) {
            for (auto group : splitStr(groupsString, ',')) {
                groups.insert(group);
            }
        }

        groupsToDump_ = groups;
    }

    if (data_.empty()) {
        return String::EMPTY;
    }

    std::map records{data_.begin(), data_.end()};
    std::string result{};

    for (const auto &[key, value] : records) {
        auto group = key.substr(0, key.find_first_of('.'));

        if (!groupsToDump_ || groupsToDump_->contains(group)) {
            result += toString(key, value, false);
        }
    }

    const auto dxfgMetrics = isolated::metrics::IsolatedMetrics::dump();

    result += dxfgMetrics;

    if (!result.empty()) {
        if (dxfgMetrics.empty()) {
            return result.substr(0, result.size() - 1);
        }
    }

    return result;
}

DXFCPP_END_NAMESPACE