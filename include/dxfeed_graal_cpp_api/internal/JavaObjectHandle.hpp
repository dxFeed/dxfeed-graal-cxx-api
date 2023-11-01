// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include "utils/StringUtils.hpp"

#include <memory>
#include <string>

namespace dxfcpp {

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

    JavaObjectHandle(JavaObjectHandle &&) = default;
    JavaObjectHandle &operator=(JavaObjectHandle &&) = default;
    virtual ~JavaObjectHandle() noexcept = default;

    [[nodiscard]] std::string toString() const noexcept {
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

} // namespace dxfcpp