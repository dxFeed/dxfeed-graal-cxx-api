// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <bits/bits.hpp>
#include <cmath>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#ifdef WIN32
#  include <windows.h>
#endif

namespace org::ttldtor {

// Windows-specific structure for terminal management.
struct VirtualTerminal {
#ifdef WIN32
  static bool enable() {
    const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Check the validity of the handle.
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
      return false;
    }

    DWORD mode{};

    if (!GetConsoleMode(handle, &mode)) {
      return false;
    }

    if (bits::bitsAreSet(mode, ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
      return true;  // The mode is already set.
    }

    if (!SetConsoleMode(handle, bits::setBits(mode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))) {
      return false;
    }

    return true;
  }

  static bool disable() {
    const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Check the validity of the handle.
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
      return false;
    }

    DWORD mode{};

    if (!GetConsoleMode(handle, &mode)) {
      return false;
    }

    if (const auto isEnabled = bits::bitsAreSet(mode, ENABLE_VIRTUAL_TERMINAL_PROCESSING); !isEnabled) {
      return true;  // The mode is already set.
    }

    if (!SetConsoleMode(handle, bits::resetBits(mode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))) {
      return false;
    }

    return true;
  }

#else
  static bool enable() {
    return true;
  }

  static bool disable() {
    return true;
  }
#endif
};

namespace portals {
inline constexpr const char* ESC = "\x1b";
inline constexpr const char* SAVE_CURSOR_POS = "[s";

enum class Color : int {
  RESET = 0,
  BLACK = 30,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  WHITE = 37,
  DEFAULT = 39,
  BRIGHT_BLACK = 90,
  BRIGHT_RED = 91,
  BRIGHT_GREEN = 92,
  BRIGHT_YELLOW = 93,
  BRIGHT_BLUE = 94,
  BRIGHT_MAGENTA = 95,
  BRIGHT_CYAN = 96,
  BRIGHT_WHITE = 97,
};

enum class BgColor : int {
  RESET = 0,
  BLACK = 40,
  RED = 41,
  GREEN = 42,
  YELLOW = 43,
  BLUE = 44,
  MAGENTA = 45,
  CYAN = 46,
  WHITE = 47,
  DEFAULT = 49,
  BRIGHT_BLACK = 100,
  BRIGHT_RED = 101,
  BRIGHT_GREEN = 102,
  BRIGHT_YELLOW = 103,
  BRIGHT_BLUE = 104,
  BRIGHT_MAGENTA = 105,
  BRIGHT_CYAN = 106,
  BRIGHT_WHITE = 107,
};

inline std::string saveCursorPos() {
  return std::string(ESC) + SAVE_CURSOR_POS;
}

inline std::string restoreCursorPos() {
  return std::string{} + ESC + "[u";
}

inline std::string hideCursor() {
  return std::string{} + ESC + "[?25l";
}

inline std::string showCursor() {
  return std::string{} + ESC + "[?25h";
}

inline std::string moveToColumn(std::size_t column) {
  return std::string{} + ESC + "[" + std::to_string(column) + "G";
}

inline std::string moveToLineBegin() {
  return moveToColumn(0);
}

inline std::string moveTo(std::size_t line, std::size_t column) {
  return std::string{} + ESC + "[" + std::to_string(line) + ";" + std::to_string(column) + "H";
}

inline std::string writeTo(std::size_t line, std::size_t column, const std::string& text) {
  return moveTo(line, column) + text;
}

inline std::string setColor(Color color) {
  return std::string{} + ESC + "[1;" + std::to_string(static_cast<int>(color)) + "m";
}

inline std::string setBgColor(BgColor bgColor) {
  return std::string{} + ESC + "[1;" + std::to_string(static_cast<int>(bgColor)) + "m";
}

inline std::string setColor(Color color, BgColor bgColor) {
  return std::string{} + ESC + "[1;" + std::to_string(static_cast<int>(color)) + ";" +
         std::to_string(static_cast<int>(bgColor)) + "m";
}

inline std::string setColor(std::uint8_t colorIndex) {
  return std::string{} + ESC + "[38;5;" + std::to_string(colorIndex) + "m";
}

inline std::string setBgColor(std::uint8_t bgColorIndex) {
  return std::string{} + ESC + "[48;5;" + std::to_string(bgColorIndex) + "m";
}

inline std::string setColor(std::uint8_t colorIndex, std::uint8_t bgColorIndex) {
  return setColor(colorIndex) + setBgColor(bgColorIndex);
}

inline std::string setColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return std::string{} + ESC + "[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

inline std::string setBgColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return std::string{} + ESC + "[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

inline std::string setColor(const std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>& colorRgb,
                            const std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>& bgColorRgb) {
  return setColor(std::get<0>(colorRgb), std::get<1>(colorRgb), std::get<2>(colorRgb)) +
         setBgColor(std::get<0>(bgColorRgb), std::get<1>(bgColorRgb), std::get<2>(bgColorRgb));
}

inline std::string resetFormat() {
  return std::string{} + ESC + "[0m";
}

}  // namespace portals

}  // namespace org::ttldtor