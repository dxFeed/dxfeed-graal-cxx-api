// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/CEntryPointErrors.hpp"
#include "./RuntimeException.hpp"

#include <stdexcept>
#include <string>

/**
 * \addtogroup dxfcpp_exceptions
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The wrapper over CEntryPointErrorsEnum, the error code returned by GraalVM.
 */
struct DXFCPP_EXPORT GraalException : RuntimeException {
    /**
     * Constructs an exception.
     *
     * @param entryPointErrorsEnum The error code returned by GraalVM.
     */
    explicit GraalException(CEntryPointErrorsEnum entryPointErrorsEnum);

    /**
     * Constructs an exception of the failed creation of the GraalVM isolate. The message has the code, its text and
     * the details.
     *
     * @param entryPointErrorsEnum The error code returned by GraalVM, or the code of the failure of the isolate arguments.
     * @param details The reason of the failure. It must not contain the values of the isolate arguments.
     */
    GraalException(CEntryPointErrorsEnum entryPointErrorsEnum, std::string details);

    GraalException(const GraalException &other) noexcept;

    GraalException &operator=(const GraalException &other) = default;

    private:
    static std::string createMessage(CEntryPointErrorsEnum entryPointErrorsEnum);
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()