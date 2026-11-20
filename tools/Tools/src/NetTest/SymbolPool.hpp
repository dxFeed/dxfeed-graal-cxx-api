// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The symbol set of NetTest: the symbols of an IPF file or generated ones.

#pragma once

#include "Args.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest {

/// The symbols of a set; shared by the event pools of all instances.
using Symbols = std::shared_ptr<const std::vector<std::string>>;

/// The result of building the set: the symbols, or the error; the warning may come with either. The caller prints the
/// warning as `NetTest: warning: <text>` and the error as `NetTest: <text>` on stderr, and ends the run with the exit
/// code 1 on an error.
struct SymbolPoolResult {
    bool ok = false;
    Symbols symbols{};
    std::string error{};
    std::string warning{};
};

/// Generates the `total` distinct symbols of QD NetTest (1-6 capital letters, now and then a digit) in the order of
/// first appearance: the same call gives the same symbols on every run and platform. Returns an empty vector when
/// `total` is below 1 or above 100000000, the largest -S.
/// @param draws if not null, receives the number of the symbols drawn, duplicates included.
std::vector<std::string> generateSymbols(std::int64_t total, std::int64_t *draws = nullptr);

/// Reads the symbols of the profiles of the IPF file in file order, at most `total`. A file with fewer symbols gives
/// the smaller set and a warning; no symbols, a missing or an unreadable file is an error.
SymbolPoolResult readSymbolsFromFile(const std::string &file, std::int64_t total);

/// Builds the set of the settings: from the IPF file when the settings name one, generated otherwise.
SymbolPoolResult buildSymbolPool(const Settings &settings);

} // namespace dxfcpp::tools::nettest
