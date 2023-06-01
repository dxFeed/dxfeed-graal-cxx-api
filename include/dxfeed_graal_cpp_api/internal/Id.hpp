// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include <atomic>
#include <cstdint>

namespace dxfcpp {

template <typename T> struct Id {
    using ValueType = std::size_t;

  private:
    const ValueType value_{};

    explicit Id(ValueType value) : value_{value} {}

  public:
    static Id<T> getNext() {
        static std::atomic<ValueType> value{};

        return Id<T>{value++};
    }

    [[nodiscard]] ValueType getValue() const { return value_; }

    explicit operator ValueType() const { return value_; }

    static Id<T> from(ValueType value) { return Id<T>{value}; }

    template <typename U> bool operator==(const Id<U> &id) const { return getValue() == id.getValue(); }

    template <typename U> auto operator<=>(const Id<U> &id) const { return getValue() <=> id.getValue(); }
};
} // namespace dxfcpp

template <typename T> struct std::hash<dxfcpp::Id<T>> {
    std::size_t operator()(const dxfcpp::Id<T> &id) const noexcept { return id.getValue(); }
};