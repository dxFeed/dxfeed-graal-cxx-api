// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include <memory>
#include <utility>
#include <cstdint>

namespace dxfcpp {

struct DXFCPP_EXPORT EventClassList {
    template <typename EventTypeIt>
    static std::unique_ptr<EventClassList> create(EventTypeIt begin, EventTypeIt end) noexcept {
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
            list->set(i, it->getId());
        }

        return list;
    }

    void set(std::size_t index, std::uint32_t id) noexcept;

    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;

    void *getHandler() noexcept;

    ~EventClassList() noexcept;

  private:
    static std::unique_ptr<EventClassList> create(std::size_t size) noexcept;

    EventClassList() noexcept;

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}