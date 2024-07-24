// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/Common.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "RuntimeException.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * A wrapper over the interceptable Java exceptions thrown by the dxFeed Native Graal SDK
 */
struct DXFCPP_EXPORT JavaException : RuntimeException {
    /**
     * Creates an exception using Java message, className and stack trace. Also uses current stack trace.
     *
     * @param message Java message.
     * @param className Java class name.
     * @param stackTrace Java stack trace.
     */
    JavaException(const StringLikeWrapper &message, const StringLikeWrapper &className,
                  const StringLikeWrapper &stackTrace);

    /**
     * Creates an exception using native (GraalVM) Java exception handle
     *
     * @param exceptionHandle The native Java exception handle.
     * @return An exception.
     */
    static JavaException create(void *exceptionHandle);

    /// Throws a JavaException if it exists (i.e. intercepted by Graal SDK)
    static void throwIfJavaThreadExceptionExists();

    static void throwException();

    template <typename T> static constexpr T *throwIfNullptr(T *v) {
        if (v == nullptr) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr const T *throwIfNullptr(const T *v) {
        if (v == nullptr) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfLessThanZero(T v) {
        if (v < T(0)) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfMinusOne(T v) {
        if (v == T(-1)) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfMinusMin(T v) {
        if (v == -std::numeric_limits<T>::min()) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfMinusInf(T v) {
        if (v == -std::numeric_limits<T>::infinity()) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()