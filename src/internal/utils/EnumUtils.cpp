// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/internal/utils/EnumUtils.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"

#include <string>

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wunused-variable")
DXFCXX_DISABLE_GCC_WARNINGS("-Wmaybe-uninitialized")
DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wdeprecated-declarations")
#include <range/v3/all.hpp>
DXFCXX_DISABLE_CLANG_WARNINGS_POP()

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
DXFCXX_DISABLE_MSC_WARNINGS_POP()
DXFCXX_DISABLE_GCC_WARNINGS_POP()