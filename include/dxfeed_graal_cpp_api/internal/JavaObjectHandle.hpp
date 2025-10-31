// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./utils/StringUtils.hpp"
#include "./utils/debug/Debug.hpp"

#include "../isolated/internal/IsolatedObject.hpp"

#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT JavaObject {
    static std::string toString(void *handle);

    static std::size_t hashCode(void *handle) noexcept;

    static bool equals(void *objectHandle1, void *objectHandle2);
};

template <typename T> struct JavaObjectHandle final {
#if DXFCPP_DEBUG == 1
    static std::string getDebugName() {
        return typeid(JavaObjectHandle<T>).name();
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
        }

        if (!handle) {
            return;
        }

        const auto result = isolated::internal::IsolatedObject::release(handle);
        ignoreUnused(result);

        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " +
                            dxfcpp::toString(result));
        }
    }

    using Impl = std::unique_ptr<void, decltype(&deleter)>;

    explicit JavaObjectHandle(void *handle = nullptr) noexcept
        // ReSharper disable once CppRedundantTypenameKeyword
        : impl_{typename Impl::pointer(handle), &deleter} {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandle(const JavaObjectHandle &) = delete;

    JavaObjectHandle(JavaObjectHandle &&other) noexcept : impl_{std::move(other.impl_)} {
    }

    JavaObjectHandle &operator=(const JavaObjectHandle &) = delete;

    JavaObjectHandle &operator=(JavaObjectHandle &&other) noexcept {
        impl_ = std::move(other.impl_);

        return *this;
    }

    ~JavaObjectHandle() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_) {
            return dxfcpp::toString(impl_.get());
        }

        return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(impl_);
    }

    private:
    Impl impl_;
};

template <typename T> struct JavaObjectHandleList final {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("JavaObjectHandleList<") + typeid(T).name() + ">";
    }
#endif

    using Type = T;

    static DXFCPP_EXPORT void deleter(void *handle) noexcept {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
        }

        if (!handle) {
            return;
        }

        const auto result = isolated::internal::IsolatedObject::List::release(handle);
        ignoreUnused(result);

        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " +
                            dxfcpp::toString(result));
        }
    }

    using Impl = std::unique_ptr<void, decltype(&deleter)>;

    explicit JavaObjectHandleList(void *handle = nullptr) noexcept
        // ReSharper disable once CppRedundantTypenameKeyword
        : impl_{typename Impl::pointer(handle), &deleter} {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandleList(const JavaObjectHandleList &) = delete;

    JavaObjectHandleList(JavaObjectHandleList &&other) noexcept : impl_{std::move(other.impl_)} {
    }

    JavaObjectHandleList &operator=(const JavaObjectHandleList &) = delete;

    JavaObjectHandleList &operator=(JavaObjectHandleList &&other) noexcept {
        impl_ = std::move(other.impl_);

        return *this;
    }

    ~JavaObjectHandleList() noexcept = default;

    [[nodiscard]] std::string toString() const {
        if (impl_) {
            return dxfcpp::toString(impl_.get());
        }

        return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        return impl_.get();
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(impl_);
    }

    private:
    Impl impl_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()