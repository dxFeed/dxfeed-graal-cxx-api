// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include <stdexcept>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * A wrapper over the interceptable Java exceptions thrown by the dxFeed Native Graal SDK
 */
struct DXFCPP_EXPORT JavaException : public std::runtime_error {
    JavaException(const std::string &message, const std::string &className, std::string stackTrace);

    static void throwIfJavaThreadExceptionExists();

    static void throwException();

    template <typename T>
    static constexpr T* throwIfNullptr(T* v) {
        if (v == nullptr) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T>
    static constexpr const T* throwIfNullptr(const T* v) {
        if (v == nullptr) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T>
    static constexpr T throwIfLessThanZero(T v) {
        if (v < T(0)) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    /**
     * @return Java exception's stack trace.
     */
    const std::string& getStackTrace() const&;

  private:

    std::string stackTrace_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()