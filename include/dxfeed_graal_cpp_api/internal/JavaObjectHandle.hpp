// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include "utils/StringUtils.hpp"

#include <memory>
#include <string>

namespace dxfcpp {

struct DXFCPP_EXPORT DefaultDeleter {
    static void deleter(void *handle) noexcept;
};

template <typename T, typename Deleter = DefaultDeleter> struct JavaObjectHandle {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("JavaObjectHandle<") + typeid(T).name() + ">";
    }
#endif

    using Type = T;
    using DeleterType = Deleter;

    explicit JavaObjectHandle(void *handle = nullptr) noexcept : mutex_(), handle_{handle} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "(handle = " + dxfcpp::toString(handle) + ")");
        }
    }

    JavaObjectHandle(const JavaObjectHandle &) = delete;

    JavaObjectHandle(JavaObjectHandle && otherJavaObjectHandle) noexcept {
        std::lock_guard lock(otherJavaObjectHandle.mutex_);

        handle_ = otherJavaObjectHandle.handle_;
    }

    JavaObjectHandle &operator=(const JavaObjectHandle &) = delete;

    JavaObjectHandle &operator=(JavaObjectHandle && otherJavaObjectHandle) noexcept {
        std::scoped_lock lock(mutex_, otherJavaObjectHandle.mutex_);

        DeleterType::deleter(handle_);

        handle_ = otherJavaObjectHandle.handle_;
        otherJavaObjectHandle.handle_ = nullptr;

        return *this;
    }

    virtual ~JavaObjectHandle() noexcept {
        DeleterType::deleter(handle_);
    }

    [[nodiscard]] std::string toString() const noexcept {
        std::lock_guard lock(mutex_);

        if (handle_)
            return dxfcpp::toString(handle_);
        else
            return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept {
        std::lock_guard lock(mutex_);

        return handle_;
    }

    explicit operator bool() const noexcept {
        std::lock_guard lock(mutex_);

        return handle_ != nullptr;
    }

  private:
    mutable std::mutex mutex_{};
    void *handle_{nullptr};
};

} // namespace dxfcpp