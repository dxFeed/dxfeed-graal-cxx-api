// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The run of NetTest: the endpoints, the publishing and the reports.

#pragma once

#include "Args.hpp"

#include <iosfwd>

namespace dxfcpp::tools::nettest {

/// Runs the tool with the validated settings: the statistics lines go to `out`, the messages of a failure to `err`.
/// Returns the exit code of the process.
int run(const Settings &settings, std::ostream &out, std::ostream &err);

} // namespace dxfcpp::tools::nettest
