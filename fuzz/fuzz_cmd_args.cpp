// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz CmdArgsUtils parsers that do not touch Graal: parseProperties (noexcept), parseEventSources (throws
// InvalidArgumentException for an invalid source name).
// parseSymbols/parseCandleSymbols call IsolatedTools (Graal) and are not fuzzed here.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    const std::string s(reinterpret_cast<const char *>(data), size);

    (void)dxfcpp::CmdArgsUtils::parseProperties(s);

    try {
        (void)dxfcpp::CmdArgsUtils::parseEventSources(s);
    } catch (const dxfcpp::InvalidArgumentException &) {
    }

    return 0;
}
