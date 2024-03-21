// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <utf8.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

using namespace std::literals;

DXFCPP_BEGIN_NAMESPACE

namespace enum_utils {

std::string getEventTypeEnumNamesList(std::string separator) noexcept {
    return EventTypeEnum::ALL_BY_NAME | ranges::views::keys | ranges::views::join(separator) | ranges::to<std::string>;
}

std::string getMarketEventTypeEnumNamesList(std::string separator) noexcept {
    return EventTypeEnum::ALL_BY_NAME | ranges::views::filter([](const auto &et) {
               return et.second.get().isMarket();
           }) |
           ranges::views::keys | ranges::views::join(separator) | ranges::to<std::string>;
}

std::string getEventTypeEnumClassNamesList(std::string separator) noexcept {
    return EventTypeEnum::ALL_BY_CLASS_NAME | ranges::views::keys | ranges::views::join(separator) |
           ranges::to<std::string>;
}

std::string getMarketEventTypeEnumClassNamesList(std::string separator) noexcept {
    return EventTypeEnum::ALL_BY_CLASS_NAME | ranges::views::filter([](const auto &et) {
               return et.second.get().isMarket();
           }) |
           ranges::views::keys | ranges::views::join(separator) | ranges::to<std::string>;
}

} // namespace enum_utils

DXFCPP_END_NAMESPACE