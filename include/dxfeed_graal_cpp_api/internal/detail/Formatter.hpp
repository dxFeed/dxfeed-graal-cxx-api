// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include "../../event/EventType.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>
#include <fmt/ostream.h>

DXFCPP_BEGIN_NAMESPACE

DXFCPP_END_NAMESPACE

template <> struct fmt::formatter<dxfcpp::EventType> : ostream_formatter {};

template <> struct fmt::formatter<std::shared_ptr<dxfcpp::EventType>> : ostream_formatter {};

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<dxfcpp::EventType, T>::value, char>> : ostream_formatter {};

DXFCXX_DISABLE_MSC_WARNINGS_POP()