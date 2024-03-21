// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/CEntryPointErrors.hpp"

#include <stdexcept>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * The wrapper over CEntryPointErrorsEnum, the error code returned by GraalVM.
 */
struct DXFCPP_EXPORT GraalException : public std::runtime_error {
    /**
     * Constructs an exception.
     *
     * @param entryPointErrorsEnum The error code returned by GraalVM.
     */
    GraalException(CEntryPointErrorsEnum entryPointErrorsEnum);

    /**
     * @return dxFeed Graal CXX API stack trace
     */
    const std::string& getStackTrace() const&;

  private:
    static inline std::string createMessage(CEntryPointErrorsEnum entryPointErrorsEnum) {
        auto result = CEntryPointErrorsEnumToStr(entryPointErrorsEnum);

        if (!result) {
            return std::string("Unknown error with error code: ") +
                   std::to_string(static_cast<std::size_t>(entryPointErrorsEnum));
        }

        return result;
    }

    std::string stackTrace_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()