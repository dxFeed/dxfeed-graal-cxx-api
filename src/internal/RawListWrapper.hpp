// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./Common.hpp"

#include <cstdint>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

template <typename T>
concept RawGraalList = requires(T list) {
    { list.size };
    { list.elements };
};

template <RawGraalList List> struct RawGraalListTraits {
    using SizeType = typename std::decay_t<decltype(List::size)>;
    using ElementType = typename std::decay_t<RemoveAllPointers<decltype(List::elements)>>;
};

template <RawGraalList List, auto ElementSetter> struct RawListWrapper {
    mutable List list_;

    RawListWrapper() noexcept : list_{0, nullptr} {
    }

    void set(std::size_t index, const auto &value) const noexcept {
        if (list_.size == 0) {
            return;
        }

        if (index < static_cast<std::size_t>(list_.size)) {
            ElementSetter(list_, index, value);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept {
        return list_.size == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return static_cast<std::size_t>(list_.size);
    }

    void *getHandle() noexcept {
        return static_cast<void *>(&list_);
    }

    void init(std::size_t size) {
        if (size <= 0) {
            return;
        }

        list_.size = static_cast<typename RawGraalListTraits<List>::SizeType>(size);
        list_.elements = new typename RawGraalListTraits<List>::ElementType *[static_cast<std::size_t>(list_.size)];

        for (typename RawGraalListTraits<List>::SizeType i = 0; i < list_.size; i++) {
            list_.elements[i] = new typename RawGraalListTraits<List>::ElementType{};
        }
    }

    void release() {
        if (list_.size == 0 || list_.elements == nullptr) {
            return;
        }

        for (auto i = list_.size - 1; i >= 0; i--) {
            delete list_.elements[i];
        }

        delete[] list_.elements;
        list_.size = 0;
        list_.elements = nullptr;
    }

    ~RawListWrapper() noexcept {
        release();
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()