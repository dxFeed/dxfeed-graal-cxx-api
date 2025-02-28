// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "OptionSeries.hpp"

#include <set>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Set of option series for a single product or underlying symbol.
 *
 * <h3>Threads and clocks</h3>
 *
 * This class is <b>NOT</b> thread-safe and cannot be used from multiple threads without external synchronization.
 *
 * @tparam T The type of option instrument instances
 */
template <typename T> class OptionChain final {
    friend class OptionChainBuilder<T>;

    std::string symbol_{};
    std::set<OptionSeries<T>> series_{};

    explicit OptionChain(std::string symbol) : symbol_(std::move(symbol)) {
    }

    void addOption(const OptionSeries<T>& series, bool isCall, double strike, const T& option) {
        auto& os = series_.find(series);

        if (os == series_.end()) {
            os = series_.insert(series).first;
        }

        os->addOption(isCall, strike, option);
    }

    void addOption(const OptionSeries<T>& series, bool isCall, double strike, std::shared_ptr<T> option) {
        addOption(series, isCall, strike, *option);
    }
public:
    /**
     * Returns symbol (product or underlying) of this option chain.
     *
     * @return symbol (product or underlying) of this option chain.
     */
    const std::string &getSymbol() const & {
        return symbol_;
    }

    /**
     * Returns a sorted set of option series of this option chain.
     *
     * @return sorted set of option series of this option chain.
     */
    std::set<OptionSeries<T>> getSeries() {
        return series_;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()