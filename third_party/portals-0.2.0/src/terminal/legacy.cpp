// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/legacy.hpp>

#include <format>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace portals::legacy {

std::expected<void, TerminalError> VirtualTerminal::tryEnable() noexcept {
#if defined(_WIN32)
  const auto output = GetStdHandle(STD_OUTPUT_HANDLE);

  if (output == INVALID_HANDLE_VALUE) {
    return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, static_cast<int>(GetLastError())});
  }

  if (output == nullptr) {
    return std::unexpected(TerminalError{TerminalErrorKind::INVALID_OPTIONS, 0});
  }

  DWORD mode{};

  if (!GetConsoleMode(output, &mode)) {
    return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, static_cast<int>(GetLastError())});
  }

  // VT interpretation also requires processed output. Input stays cooked so
  // the original sample's std::cin.get() semantics are preserved.
  constexpr DWORD REQUIRED_OUTPUT_MODE = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

  if ((mode & REQUIRED_OUTPUT_MODE) != REQUIRED_OUTPUT_MODE && !SetConsoleMode(output, mode | REQUIRED_OUTPUT_MODE)) {
    return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, static_cast<int>(GetLastError())});
  }
#endif

  return {};
}

bool VirtualTerminal::enable() noexcept {
  return tryEnable().has_value();
}

std::string moveTo(std::size_t line, std::size_t column) {
  return std::format("\x1b[{};{}H", line, column);
}

std::string hideCursor() {
  return "\x1b[?25l";
}

std::string showCursor() {
  return "\x1b[?25h";
}

} // namespace portals::legacy
