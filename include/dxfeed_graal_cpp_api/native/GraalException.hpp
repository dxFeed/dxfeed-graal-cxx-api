// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4275)

#include "../internal/CEntryPointErrors.hpp"

#include <stdexcept>
#include <string>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT GraalException : public std::runtime_error {
    explicit GraalException(CEntryPointErrorsEnum entryPointErrorsEnum)
        : std::runtime_error(createMessage(entryPointErrorsEnum)) {
    }

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