// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/GraalException.hpp"

DXFCPP_BEGIN_NAMESPACE

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum)
    : RuntimeException(createMessage(entryPointErrorsEnum)) {
}

GraalException::GraalException(const GraalException &other) noexcept = default;
std::string GraalException::createMessage(CEntryPointErrorsEnum entryPointErrorsEnum) {
    const auto result = CEntryPointErrorsEnumToStr(entryPointErrorsEnum);

    if (!result) {
        return std::string("Unknown error with error code: ") +
               std::to_string(static_cast<std::size_t>(entryPointErrorsEnum));
    }

    return result;
}

DXFCPP_END_NAMESPACE