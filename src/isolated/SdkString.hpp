// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

#include <string>
#include <string_view>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated {

/**
 * A NUL-terminated string for a call into the dxFeed Graal SDK (`dxfg_*` functions take `const char *`).
 *
 * This class and `createCString()` (for SDK structure fields that outlive the call) are the only ways a user string
 * reaches the SDK. Every conversion goes through them, so a check of the characters has one place to live.
 *
 * Lifetime: the object is used as a temporary inside the call expression,
 * `dxfg_foo(thread, SdkString{value}.c_str())`. The temporary lives until the end of the full expression, and the
 * `runGraalFunction*` helpers call the SDK synchronously, so the pointer is valid for the whole call. Do not keep
 * `c_str()` after the full expression: the object can be neither copied nor moved.
 *
 * Embedded NUL: the SDK reads the characters up to the first NUL, so a string with an embedded NUL is cut there.
 *
 * A `std::string_view` and a `const char *` are copied: a view cannot tell whether a NUL follows its last character
 * (a `char` array without NUL, a full `std::vector<char>`), so its terminator is never read. A `const std::string &`
 * is borrowed without a copy, and it must outlive the full expression, as the owner of the characters does.
 * A temporary `std::string` is moved into the object.
 */
class SdkString final {
    // MDAPI-32: validate UTF-8 here.

    std::string owned_{};
    const char *ptr_;

    public:
    /// Copies the characters of the view; a terminator is added.
    explicit SdkString(std::string_view s) : owned_{s}, ptr_{owned_.c_str()} {
    }

    /// Copies a NUL-terminated string; a null pointer is an empty string.
    explicit SdkString(const char *s) : SdkString{s == nullptr ? std::string_view{} : std::string_view{s}} {
    }

    /// Borrows the characters of the string, which is already NUL-terminated. The string must outlive this object.
    explicit SdkString(const std::string &s) noexcept : ptr_{s.c_str()} {
    }

    /// Takes over a temporary string: borrowing it would dangle at the end of the constructor's call.
    explicit SdkString(std::string &&s) noexcept : owned_{std::move(s)}, ptr_{owned_.c_str()} {
    }

    SdkString(const SdkString &) = delete;
    SdkString(SdkString &&) = delete;
    SdkString &operator=(const SdkString &) = delete;
    SdkString &operator=(SdkString &&) = delete;
    ~SdkString() = default;

    /// @return The NUL-terminated characters, valid until the end of the full expression that created the object.
    [[nodiscard]] const char *c_str() const noexcept {
        return ptr_;
    }
};

} // namespace isolated

DXFCPP_END_NAMESPACE
