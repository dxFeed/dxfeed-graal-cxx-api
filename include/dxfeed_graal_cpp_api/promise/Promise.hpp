// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

struct TimeSeriesEvent;
struct LastingEvent;

struct PromiseImpl {

};

template <typename T> struct Promise {
};

template <class T, class U>
concept Derived = std::is_base_of_v<U, T>;

template <Derived<LastingEvent> T> struct Promise<std::shared_ptr<T>> {

};

template <Derived<TimeSeriesEvent> T> struct Promise<std::shared_ptr<T>> {

};

template <Derived<TimeSeriesEvent> T> struct Promise<std::vector<std::shared_ptr<T>>> {

};


//
// Promise<std::shared_ptr<LastingEvent>> x{};
// auto _ = x.z();
//
// template <typename T, typename Enable = void> struct PromiseSFINAE {
//     int x() {
//         return {};
//     }
// };
//
// template <typename T>
// struct PromiseSFINAE<T, typename std::enable_if<std::is_base_of<LastingEvent, typename T::element_type>::value>::type> {
//     int z() {
//         return 0;
//     }
// };
//
// PromiseSFINAE<std::shared_ptr<LastingEvent>> xx{};
// auto __ = xx.z();

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()