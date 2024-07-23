// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "utils/StringUtils.hpp"

#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

template <typename T> struct JavaObjectHandle {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("JavaObjectHandle<") + typeid(T).name() + ">";
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept;
    explicit JavaObjectHandle(void *handle = nullptr) noexcept : impl_{handle, &deleter} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandle(const JavaObjectHandle &) = delete;
    JavaObjectHandle(JavaObjectHandle &&) noexcept = default;
    JavaObjectHandle &operator=(const JavaObjectHandle &) = delete;
    JavaObjectHandle &operator=(JavaObjectHandle &&) noexcept = default;
    virtual ~JavaObjectHandle() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_)
            return dxfcpp::toString(impl_.get());
        else
            return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(impl_);
    }

  private:
    std::unique_ptr<void, decltype(&deleter)> impl_;
};

template <typename T> struct JavaObjectHandleList {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("JavaObjectHandleList<") + typeid(T).name() + ">";
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept;
    explicit JavaObjectHandleList(void *handle = nullptr) noexcept : impl_{handle, &deleter} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandleList(const JavaObjectHandleList &) = delete;
    JavaObjectHandleList(JavaObjectHandleList &&) noexcept = default;
    JavaObjectHandleList &operator=(const JavaObjectHandleList &) = delete;
    JavaObjectHandleList &operator=(JavaObjectHandleList &&) noexcept = default;
    virtual ~JavaObjectHandleList() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_)
            return dxfcpp::toString(impl_.get());
        else
            return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(impl_);
    }

  private:
    std::unique_ptr<void, decltype(&deleter)> impl_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()