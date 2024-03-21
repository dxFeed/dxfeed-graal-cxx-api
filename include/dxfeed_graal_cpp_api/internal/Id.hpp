// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <atomic>
#include <cstdint>

DXFCPP_BEGIN_NAMESPACE

template <typename T> struct Id {
    using ValueType = std::size_t;
    const static Id UNKNOWN;

  private:
    ValueType value_{};

    explicit Id(ValueType value) : value_{value} {
    }

  public:
    static Id<T> getNext() {
        static std::atomic<ValueType> value{1};

        return Id<T>{value++};
    }

    [[nodiscard]] ValueType getValue() const {
        return value_;
    }

    explicit operator ValueType() const {
        return value_;
    }

    static Id<T> from(ValueType value) {
        return Id<T>{value};
    }

    template <typename H> static Id<T> from(H *handle) {
        return Id<T>{dxfcpp::bit_cast<ValueType>(handle)};
    }

    template <typename H> static Id<T> from(const H *handle) {
        return Id<T>{dxfcpp::bit_cast<ValueType>(handle)};
    }

    template <typename U> bool operator==(const Id<U> &id) const {
        return getValue() == id.getValue();
    }

    template <typename U> auto operator<=>(const Id<U> &id) const {
        return getValue() <=> id.getValue();
    }
};

template <typename T> const Id<T> Id<T>::UNKNOWN{static_cast<Id<T>::ValueType>(-1)};

DXFCPP_END_NAMESPACE

template <typename T> struct std::hash<dxfcpp::Id<T>> {
    std::size_t operator()(const dxfcpp::Id<T> &id) const noexcept {
        return id.getValue();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()