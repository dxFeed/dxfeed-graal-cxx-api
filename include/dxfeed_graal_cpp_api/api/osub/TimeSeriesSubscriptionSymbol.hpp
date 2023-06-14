// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../symbols/SymbolWrapper.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

struct SymbolWrapper;

class DXFCPP_EXPORT TimeSeriesSubscriptionSymbol final {
    std::unique_ptr<SymbolWrapper> eventSymbol_;
    std::int64_t fromTime_;

  public:
    TimeSeriesSubscriptionSymbol(const SymbolWrapper &eventSymbol, int64_t fromTime);
    TimeSeriesSubscriptionSymbol(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept;
    TimeSeriesSubscriptionSymbol(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept;
    TimeSeriesSubscriptionSymbol &operator=(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept;
    TimeSeriesSubscriptionSymbol &operator=(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept;
    TimeSeriesSubscriptionSymbol() noexcept = default;
    virtual ~TimeSeriesSubscriptionSymbol() noexcept = default;

    const std::unique_ptr<SymbolWrapper> &getEventSymbol() const;

    int64_t getFromTime() const;

    void *toGraal() const noexcept;

    std::string toString() const noexcept;

    bool operator==(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept;

    bool operator<(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept;
};

} // namespace dxfcpp

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::TimeSeriesSubscriptionSymbol> {
    std::size_t operator()(const dxfcpp::TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
        return std::hash<std::unique_ptr<dxfcpp::SymbolWrapper>>{}(timeSeriesSubscriptionSymbol.getEventSymbol());
    }
};