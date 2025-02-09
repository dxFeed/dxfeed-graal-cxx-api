// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "Common.hpp"
#include "NonCopyable.hpp"
#include "utils/StringUtils.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

struct MetricsManager : private NonCopyable<MetricsManager> {
    using Value = std::variant<std::string, double, std::int64_t>;

  private:
    std::mutex mtx_{};
    std::unordered_map<std::string, Value> data_{};

    void setImpl(const std::string &name, const Value &value) {
        data_[name] = value;
    }

    template <typename T> void addImpl(const std::string &name, T &&value) {
        if (!data_.contains(name)) {
            setImpl(name, Value{value});

            return;
        }

        setImpl(name, std::visit(Overloads{
                                     [&value](const std::string &s) {
                                         return Value{s + std::to_string(value)};
                                     },
                                     [&value](auto d) {
                                         return Value{d + value};
                                     },
                                 },
                                 data_.at(name)));
    }

  public:
    static std::string toString(const Value &value) {
        return std::visit(Overloads{
                              [](const std::string &s) {
                                  return s;
                              },
                              [](auto n) {
                                  return std::to_string(n);
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

        return toString(data_.at(name));
    }

    double getAsDouble(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (!data_.contains(name)) {
            return math::NaN;
        }

        return std::visit(Overloads{
                              [](const std::string &s) {
                                  return std::stod(s);
                              },
                              [](auto n) {
                                  return double(n);
                              },
                          },
                          data_.at(name));
    }

    std::int64_t getAsI64(const std::string &name) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (!data_.contains(name)) {
            return 0;
        }

        return std::visit(Overloads{
                              [](const std::string &s) {
                                  return static_cast<std::int64_t>(std::stoll(s));
                              },
                              [](auto n) {
                                  return static_cast<std::int64_t>(n);
                              },
                          },
                          data_.at(name));
    }

    void set(const std::string &name, const Value &value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        setImpl(name, value);
    }

    void add(const std::string &name, double value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        addImpl(name, value);
    }

    void add(const std::string &name, Integral auto value) {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        addImpl(name, value);
    }

    std::string dump() {
        std::lock_guard<std::mutex> lockGuard{mtx_};

        if (data_.empty()) {
            return String::EMPTY;
        }

        std::map<std::string, Value> records{data_.begin(), data_.end()};
        std::string result{};

        for (const auto &[key, value] : records) {
            result += key + ": " + toString(value) + '\n';
        }

        return result.substr(0, result.size() - 1);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()