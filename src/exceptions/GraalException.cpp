// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/GraalException.hpp"

#include <string>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

namespace {

std::string createCreationMessage(CEntryPointErrorsEnum entryPointErrorsEnum, const std::string &details) {
    const auto text = CEntryPointErrorsEnumToStr(entryPointErrorsEnum);

    return "Failed to create the GraalVM isolate, error " +
           std::to_string(static_cast<std::underlying_type_t<CEntryPointErrorsEnum>>(entryPointErrorsEnum)) + ": " +
           (text ? text : "unknown error.") + " " + details;
}

} // namespace

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum)
    : RuntimeException(createMessage(entryPointErrorsEnum)) {
}

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum, std::string details)
    : RuntimeException(createCreationMessage(entryPointErrorsEnum, details)) {
}

GraalException::GraalException(const GraalException &other) noexcept : RuntimeException(other) {
}

std::string GraalException::createMessage(CEntryPointErrorsEnum entryPointErrorsEnum) {
    const auto result = CEntryPointErrorsEnumToStr(entryPointErrorsEnum);

    if (!result) {
        return std::string("Unknown error with error code: ") +
               std::to_string(static_cast<std::size_t>(entryPointErrorsEnum));
    }

    return result;
}

DXFCPP_END_NAMESPACE