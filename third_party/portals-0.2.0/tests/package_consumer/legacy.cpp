// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals.hpp>
#include <portals/legacy.hpp>

#include <format>
#include <sstream>
#include <string>
#include <type_traits>

int main() {
  static_assert(std::is_same_v<org::ttldtor::VirtualTerminal, portals::legacy::VirtualTerminal>);

  std::ostringstream frame;
  frame << org::ttldtor::portals::hideCursor() << std::format("{}{}{}", portals::legacy::moveTo(4, 2), "Price: ", "42")
        << org::ttldtor::portals::showCursor();

  const std::string expected = "\x1b[?25l\x1b[4;2HPrice: 42\x1b[?25h";

  return frame.str() == expected ? 0 : 1;
}
