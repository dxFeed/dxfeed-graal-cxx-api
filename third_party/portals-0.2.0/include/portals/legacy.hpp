// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/terminal.hpp>

#include <cstddef>
#include <expected>
#include <string>

/** @brief Minimal streaming compatibility for the original MarketDepthModelSample.
 * This API does not run an Application, perform rendering, or own a terminal session.
 * The caller serializes and flushes output and must not mix it with an active Application.
 */
namespace portals::legacy {

/** @brief Enable VT interpretation on the process's existing standard output.
 * This stateless compatibility utility neither changes input mode nor opens a console.
 * It does not restore output mode; use the retained Application for an owned terminal lifetime.
 */
struct VirtualTerminal {
  /** @brief Checked form of the historical output-mode switch.
   * On Windows, preserve other mode bits and enable ENABLE_PROCESSED_OUTPUT and
   * ENABLE_VIRTUAL_TERMINAL_PROCESSING.
   * Calls must be serialized with other changes to the same console mode. A redirected
   * Windows stdout is rejected. On non-Windows platforms this is a successful no-op,
   * matching the old helper; it does not detect ANSI capabilities.
   * @return Success, INVALID_OPTIONS with native_code zero for an absent stdout,
   * or SYSTEM with the original Windows error code when a platform call fails.
   */
  static std::expected<void, TerminalError> tryEnable() noexcept;

  /** @brief Historical bool form, without retaining or printing an error.
   * @return True on success; false on a Windows output handle or console mode failure.
   */
  static bool enable() noexcept;
};

/** @brief Return a cursor-position sequence without performing I/O.
 * @param line VT row number, normally one-based. Zero is emitted unchanged and means row one in VT.
 * @param column VT column number, normally one-based. Zero is emitted unchanged and means column one in VT.
 * @return Owning ESC[line;columnH string, suitable for concatenation and stream insertion.
 * Coordinates are not converted from CellPoint's zero-based convention or clamped to screen bounds.
 * @throws std::bad_alloc If string allocation fails.
 */
std::string moveTo(std::size_t line, std::size_t column);

/** @brief Return the VT cursor-hide sequence; ignoring the result has no effect.
 * @return Owning ESC[?25l string; the caller must output and flush it.
 * @throws std::bad_alloc If string allocation fails.
 */
std::string hideCursor();

/** @brief Return the VT cursor-show sequence; ignoring the result has no effect.
 * @return Owning ESC[?25h string; the caller must output and flush it.
 * @throws std::bad_alloc If string allocation fails.
 */
std::string showCursor();

} // namespace portals::legacy
