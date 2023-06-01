// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../IndexedEventSource.hpp"

namespace dxfcpp {

// TODO: implement
class DXFCPP_EXPORT OrderSource final : public IndexedEventSource {

    static constexpr std::uint32_t PUB_ORDER = 0x0001U;
    static constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
    static constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0004U;
    static constexpr std::uint32_t FULL_ORDER_BOOK = 0x0008U;
    static constexpr std::uint32_t FLAGS_SIZE = 4U;

    static const std::unordered_map<std::uint32_t, std::reference_wrapper<const OrderSource>> INTERNAL_;

    static inline std::mutex MTX_{};
    const std::unordered_map<std::uint32_t, std::reference_wrapper<const OrderSource>> USER_;

    std::uint32_t pubFlags_{};
    bool builtin_{};

    OrderSource(std::uint32_t id, std::string name, std::uint32_t pubFlags) noexcept
        : IndexedEventSource(id, std::move(name)), pubFlags_{pubFlags}, builtin_{true} {}

    OrderSource(const std::string &name, std::uint32_t pubFlags) : OrderSource(composeId(name), name, pubFlags) {}

    static std::uint32_t composeId(const std::string & /*name*/) noexcept { return {}; }
};

} // namespace dxfcpp