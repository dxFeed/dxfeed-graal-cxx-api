// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum)
    : RuntimeException(createMessage(entryPointErrorsEnum)) {
}

GraalException::GraalException(const GraalException& other) noexcept = default;

DXFCPP_END_NAMESPACE