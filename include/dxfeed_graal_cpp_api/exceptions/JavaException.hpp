// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../detail/StringParam.hpp"
#include "./RuntimeException.hpp"

#include <memory>
#include <string>

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
    JavaException(StringParam message, StringParam className,
                  StringParam stackTrace);

    JavaException(const JavaException &other) noexcept;

    JavaException &operator=(const JavaException &other) = default;

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

    /// @return The class name of the Java exception.
    const std::string &getJavaClassName() const & noexcept;

    /// @return The message of the Java exception, without the prefix that what() adds.
    const std::string &getJavaMessage() const & noexcept;

    /// @return The stack trace of the Java exception.
    const std::string &getJavaStackTrace() const & noexcept;

    /// Throws a JavaException if it exists (i.e. intercepted by Graal SDK)
    static void throwIfJavaThreadExceptionExists();

    static void throwException();

    private:
    struct JavaData;

    std::shared_ptr<const JavaData> javaData_;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
