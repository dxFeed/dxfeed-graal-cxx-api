// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <portals.hpp>
#include <portals/legacy.hpp>

#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

TEST_CASE("legacy cursor helpers preserve the historical escape strings") {
  CHECK(portals::legacy::moveTo(1, 1) == "\x1b[1;1H");
  CHECK(portals::legacy::moveTo(0, 0) == "\x1b[0;0H");
  CHECK(portals::legacy::moveTo(12, 34) == "\x1b[12;34H");

  const auto maximum = std::to_string(std::numeric_limits<std::size_t>::max());
  CHECK(portals::legacy::moveTo(std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()) ==
        "\x1b[" + maximum + ";" + maximum + "H");
  CHECK(portals::legacy::hideCursor() == "\x1b[?25l");
  CHECK(portals::legacy::showCursor() == "\x1b[?25h");
}

TEST_CASE("legacy helpers compose without writing to standard streams") {
  static_assert(std::is_same_v<org::ttldtor::VirtualTerminal, portals::legacy::VirtualTerminal>);

  std::ostringstream standardOutput;
  std::ostringstream standardError;

  struct StreamRestore {
    std::streambuf *output;
    std::streambuf *error;

    ~StreamRestore() {
      std::cout.rdbuf(output);
      std::cerr.rdbuf(error);
    }
  };

  std::string sequence;

  {
    StreamRestore restore{std::cout.rdbuf(standardOutput.rdbuf()), std::cerr.rdbuf(standardError.rdbuf())};

    sequence += org::ttldtor::portals::hideCursor();
    sequence += org::ttldtor::portals::moveTo(3, 7);
    sequence += org::ttldtor::portals::showCursor();
  }

  CHECK(sequence == "\x1b[?25l\x1b[3;7H\x1b[?25h");
  CHECK(standardOutput.str().empty());
  CHECK(standardError.str().empty());
}

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>

TEST_CASE("legacy VT enable rejects missing stdout without opening a window") {
  const auto original = GetStdHandle(STD_OUTPUT_HANDLE);

  REQUIRE(SetStdHandle(STD_OUTPUT_HANDLE, nullptr) != FALSE);

  const auto result = portals::legacy::VirtualTerminal::tryEnable();
  const auto enabled = org::ttldtor::VirtualTerminal::enable();
  const auto restored = SetStdHandle(STD_OUTPUT_HANDLE, original);

  REQUIRE(restored != FALSE);
  CHECK_FALSE(result.has_value());
  CHECK_FALSE(enabled);
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().kind == portals::TerminalErrorKind::INVALID_OPTIONS);
  CHECK(result.error().native_code == 0);
}
#else
TEST_CASE("legacy VT enable is a successful no-op on non-Windows platforms") {
  CHECK(portals::legacy::VirtualTerminal::tryEnable().has_value());
  CHECK(org::ttldtor::VirtualTerminal::enable());
}
#endif
