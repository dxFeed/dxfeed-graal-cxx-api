// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include "utils/StringUtils.hpp"

#include <memory>
#include <string>

namespace dxfcpp {

template <typename T> struct JavaObjectHandler {
    using Type = T;
    static DXFCPP_EXPORT void deleter(void *handler) noexcept;
    explicit JavaObjectHandler(void *handler = nullptr) noexcept : impl_{handler, &deleter} {}

    JavaObjectHandler(JavaObjectHandler &&) = default;
    JavaObjectHandler &operator=(JavaObjectHandler &&) = default;
    virtual ~JavaObjectHandler() noexcept = default;

    [[nodiscard]] std::string toString() const noexcept {
        if (impl_)
            return dxfcpp::toString(impl_.get());
        else
            return "nullptr";
    }

    [[nodiscard]] void *get() const noexcept { return impl_.get(); }

    explicit operator bool() const noexcept { return static_cast<bool>(impl_); }

  private:
    std::unique_ptr<void, decltype(&deleter)> impl_;
};

} // namespace dxfcpp