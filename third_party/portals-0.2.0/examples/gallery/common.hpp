// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <iostream>
#include <portals/application.hpp>
#include <stdexcept>

namespace example {
/** @brief Unwrap a finite example operation or stop setup with a generic exception.
 * @tparam T Expected success type: void, or a movable value returned by ownership transfer.
 * @tparam E Expected error type; the helper deliberately does not format its value.
 * @param result Owning operation result; no reference into it is retained.
 * @return Moved success value, or no value for void.
 * @throws std::runtime_error If the operation failed; original error detail is lost.
 * Argument construction, allocation and success-value move exceptions also propagate.
 * This example-only fail-fast policy is not a production error-handling recommendation.
 */
template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    throw std::runtime_error("example setup/preparation failed");
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

/** @brief Validate and copy example text under the receiving view's display policy.
 * @param value Borrowed UTF-8 bytes, consumed synchronously.
 * @param policy Width policy captured by the owning result.
 * @return Independent validated text.
 * @throws std::runtime_error If validation fails; allocation exceptions also propagate.
 */
inline portals::GraphemeText text(std::string_view value, portals::WidthPolicy policy = {}) {
  return take(portals::GraphemeText::from(value, policy));
}

/** @brief Obtain a static diagnostic label without allocating.
 * @param phase Application phase to describe, including an unknown numeric value.
 * @return Static label; "unknown" for a value without a named phase.
 */
inline constexpr std::string_view phaseName(portals::ApplicationPhase phase) noexcept {
  using enum portals::ApplicationPhase;

  switch (phase) {
  case CREATE:
    return "create";
  case OPEN:
    return "open";
  case INPUT:
    return "input";
  case DISPATCH:
    return "dispatch";
  case PREPARE:
    return "prepare";
  case RENDER:
    return "render";
  case SUBMIT:
    return "submit";
  case FLUSH:
    return "flush";
  case STOP:
    return "stop";
  case CLOSE:
    return "close";
  case CONTROL_LATENCY:
    return "control latency observer";
  }

  return "unknown";
}

/** @brief Report a typed application failure on stderr for a CLI exit status.
 * @param result Borrowed runner result, including any independent restoration error.
 * @return Zero on success, one on failure; success produces no diagnostic.
 * Stream exceptions propagate if enabled by the caller.
 */
inline int report(const std::expected<portals::ApplicationStats, portals::ApplicationError> &result) {
  if (result) {
    return 0;
  }

  const auto &error = result.error();

  std::cerr << "Application failed in " << phaseName(error.phase) << ": ";
  std::visit(
    [](const auto &cause) {
      using T = std::decay_t<decltype(cause)>;

      if constexpr (std::same_as<T, portals::TerminalError>) {
        std::cerr << "TerminalError=" << static_cast<int>(cause.kind) << " native=" << cause.native_code
                  << " rollback=" << cause.restoration_code;
      } else if constexpr (std::same_as<T, portals::RenderError>) {
        std::cerr << "RenderError=" << static_cast<int>(cause.kind);
      } else if constexpr (std::same_as<T, portals::ApplicationFailure>) {
        std::cerr << "ApplicationFailure=" << static_cast<int>(cause);
      } else if constexpr (std::same_as<T, portals::TreeError>) {
        std::cerr << "TreeError=" << static_cast<int>(cause);
      } else if constexpr (std::same_as<T, portals::SchedulerError>) {
        std::cerr << "SchedulerError=" << static_cast<int>(cause);
      } else if constexpr (std::same_as<T, portals::InputError>) {
        std::cerr << "InputError=" << static_cast<int>(cause);
      } else {
        std::cerr << "FrameError=" << static_cast<int>(cause);
      }
    },
    error.cause);

  if (error.restoration) {
    std::cerr << " restoration.kind=" << static_cast<int>(error.restoration->kind)
              << " restoration.native=" << error.restoration->native_code;
  }

  std::cerr << '\n';

  return 1;
}

/** @brief Recognize the examples' F10 or Ctrl+C exit shortcut.
 * @param event Borrowed event; repeats are accepted and releases are ignored.
 * @return True for an exit shortcut; no event or held-key state is consumed.
 */
inline constexpr bool quitKey(const portals::InputEvent &event) {
  const auto *key = std::get_if<portals::KeyEvent>(&event);

  return key && key->action != portals::KeyAction::RELEASE &&
         (key->key == portals::InputKey::F10 || (key->modifiers.control && key->scalar == U'c'));
}
} // namespace example
