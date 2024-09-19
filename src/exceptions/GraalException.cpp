// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <boost/stacktrace.hpp>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

std::string stackTraceToString(const boost::stacktrace::stacktrace &stacktrace);

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum)
    : RuntimeException(createMessage(entryPointErrorsEnum)) {
}

GraalException::GraalException(const GraalException& other) noexcept = default;

DXFCPP_END_NAMESPACE