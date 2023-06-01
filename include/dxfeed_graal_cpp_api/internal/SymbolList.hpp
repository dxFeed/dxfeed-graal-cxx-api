// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include "../symbols/SymbolWrapper.hpp"

namespace dxfcpp {

struct SymbolList {
    template <typename SymbolIt> static std::unique_ptr<SymbolList> create(SymbolIt begin, SymbolIt end) noexcept {
        auto size = std::distance(begin, end);

        if (size <= 0) {
            return {};
        }

        auto list = create(size);

        if (!list || list->isEmpty()) {
            return {};
        }

        std::size_t i = 0;

        for (auto it = begin; it != end; it++, i++) {
            list->set(i, *it);
        }

        return list;
    }

    void set(std::size_t index, const SymbolWrapper& symbolWrapper) noexcept;

    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    void *getHandler() noexcept;

    ~SymbolList() noexcept;

    std::string toString() const noexcept;

  private:
    static std::unique_ptr<SymbolList> create(std::size_t size) noexcept;

    SymbolList() noexcept;

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}