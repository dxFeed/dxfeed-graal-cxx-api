// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

DXFCPP_BEGIN_NAMESPACE

GraalException::GraalException(CEntryPointErrorsEnum entryPointErrorsEnum, std::string stackTrace)
    : std::runtime_error(createMessage(entryPointErrorsEnum)) {
    stackTrace_ = std::move(stackTrace) + boost::stacktrace::to_string(boost::stacktrace::stacktrace());
}

const std::string &GraalException::getStackTrace() const & {
    return stackTrace_;
}

DXFCPP_END_NAMESPACE