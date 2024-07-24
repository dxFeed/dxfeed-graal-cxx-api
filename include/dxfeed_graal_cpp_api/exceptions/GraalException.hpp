// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/CEntryPointErrors.hpp"

#include "RuntimeException.hpp"

#include <stdexcept>
#include <string>

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
    GraalException(CEntryPointErrorsEnum entryPointErrorsEnum);

  private:
    static inline std::string createMessage(CEntryPointErrorsEnum entryPointErrorsEnum) {
        auto result = CEntryPointErrorsEnumToStr(entryPointErrorsEnum);

        if (!result) {
            return std::string("Unknown error with error code: ") +
                   std::to_string(static_cast<std::size_t>(entryPointErrorsEnum));
        }

        return result;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()