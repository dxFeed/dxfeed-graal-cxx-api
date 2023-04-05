// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../IndexedEventSource.hpp"

namespace dxfcpp {

// TODO: implement
template <template <typename...> typename Cache>
class OrderSourceImpl final : public IndexedEventSource {

    static inline std::mutex MTX{};
    static inline std::size_t CACHE_SIZE = 100UL;
    static inline Cache<std::uint32_t, std::shared_ptr<OrderSourceImpl<Cache>>> SOURCES_BY_ID{};
    static inline Cache<std::string, std::shared_ptr<OrderSourceImpl<Cache>>> SOURCES_BY_STRING{};

    static constexpr std::uint32_t PUB_ORDER = 0x0001U;
    static constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
    static constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0004U;
    static constexpr std::uint32_t FULL_ORDER_BOOK = 0x0008U;
    static constexpr std::uint32_t FLAGS_SIZE = 4U;


};

} // namespace dxfcpp