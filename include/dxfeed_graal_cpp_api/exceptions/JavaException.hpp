// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/utils/StringUtils.hpp"
#include "./RuntimeException.hpp"

#include <limits>
#include <type_traits>

/**
 * \addtogroup dxfcpp_exceptions
 * @{
 */

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
    JavaException(const StringLike &message, const StringLike &className,
                  const StringLike &stackTrace);

    JavaException(const JavaException &other) noexcept;

    /**
     * Creates an exception using native (GraalVM) Java exception handle
     *
     * @param exceptionHandle The native Java exception handle.
     * @return An exception.
     */
    static JavaException create(void *exceptionHandle);

    /**
     * Creates an exception from a native Java exception handle and releases the native representation.
     *
     * @param exceptionHandle The native Java exception handle.
     * @return An exception containing a copy of the native exception data.
     */
    static JavaException createAndRelease(void *exceptionHandle);

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

    // -1 of the C API functions; for an enum (state, error code), -1 of its underlying type.
    template <typename T> static constexpr bool isMinusOne(T v) noexcept {
        if constexpr (std::is_enum_v<T>) {
            using U = std::underlying_type_t<T>;

            return static_cast<U>(v) == static_cast<U>(-1);
        } else {
            return v == static_cast<T>(-1);
        }
    }

    template <typename T> static constexpr T throwIfMinusOne(T v) {
        if (isMinusOne(v)) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfMinusMin(T v) {
        if (v == std::numeric_limits<T>::min()) {
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }

    template <typename T> static constexpr T throwIfMinusInf(T v) {
        if (v < std::numeric_limits<T>::lowest()) { // -infinity
            throwIfJavaThreadExceptionExists();
        }

        return v;
    }
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
