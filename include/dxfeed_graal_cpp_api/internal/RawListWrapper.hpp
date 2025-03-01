// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "utils/StringUtils.hpp"

DXFCPP_BEGIN_NAMESPACE

struct Debugger;

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
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("RawListWrapper<") + typeid(List).name() + ", " + typeid(ElementSetter).name() + ">";
    }
#endif

    mutable List list_;

    RawListWrapper() noexcept : list_{0, nullptr} {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::()");
        }
    }

    void set(std::size_t index, const auto &value) const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::set(" + std::to_string(index) + ", " + std::to_string(value) + ")");
        }

        if (list_.size == 0) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace(getDebugName() + "::set(" + std::to_string(index) + ", " + std::to_string(value) +
                                "): list_.size == 0");
            }

            return;
        }

        if (index < static_cast<std::size_t>(list_.size)) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace(getDebugName() + "::set(" + std::to_string(index) + ", " + std::to_string(value) +
                                "): index < list_.size");
            }

            ElementSetter(list_, index, value);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::isEmpty() -> " + dxfcpp::toString(list_.size == 0));
        }

        return list_.size == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::size() -> " + std::to_string(static_cast<std::size_t>(list_.size)));
        }

        return static_cast<std::size_t>(list_.size);
    }

    void *getHandle() noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::getHandle() -> " + dxfcpp::toString(static_cast<void *>(&list_)));
        }

        return static_cast<void *>(&list_);
    }

    void init(std::size_t size) {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::init(" + std::to_string(size) + ")");
        }

        if (size <= 0) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace(getDebugName() + "::init(" + std::to_string(size) + "): size <= 0");
            }

            return;
        }

        list_.size = static_cast<typename RawGraalListTraits<List>::SizeType>(size);
        list_.elements = new typename RawGraalListTraits<List>::ElementType *[list_.size];

        for (typename RawGraalListTraits<List>::SizeType i = 0; i < list_.size; i++) {
            list_.elements[i] = new typename RawGraalListTraits<List>::ElementType{};
        }
    }

    void release() {
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::release()");
        }

        if (list_.size == 0 || list_.elements == nullptr) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace(getDebugName() + "::release(): list_.size == 0 || list_.elements == nullptr");
            }

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
        if constexpr (Debugger::traceLists) {
            Debugger::trace(getDebugName() + "::~()");
        }

        release();
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()