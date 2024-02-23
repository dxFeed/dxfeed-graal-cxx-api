// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/CEntryPointErrors.hpp"

#include <string>

namespace dxfcpp {

struct GraalException : public std::runtime_error {
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

} // namespace dxfcpp