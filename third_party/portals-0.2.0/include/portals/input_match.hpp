// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/input.hpp>

#include <algorithm>
#include <initializer_list>

namespace portals {
/** @brief Match a semantic key press or repeat without extracting the event variant.
 * @param event Borrowed input event, never retained or consumed.
 * @param key Semantic key identity; CHARACTER does not compare its Unicode scalar.
 * @return True for matching PRESS/REPEAT; false for RELEASE or a non-key event.
 * Modifiers are ignored by this overload. Use the exact-modifier overload for shortcuts.
 * This pure predicate allocates nothing and adds no routing, focus or held-key state.
 */
constexpr bool keyPressed(const InputEvent &event, InputKey key) noexcept {
  const auto *value = std::get_if<KeyEvent>(&event);

  return value && value->key == key && value->action != KeyAction::RELEASE;
}

/** @brief Match a semantic press/repeat with the complete modifier combination.
 * @param event Borrowed input event, never retained or consumed.
 * @param key Semantic identity; use KeyEvent::scalar separately for CHARACTER keys.
 * @param modifiers Exact required modifiers, including false fields.
 * @return True for a matching PRESS/REPEAT and exact modifiers; false otherwise.
 * No allocation, event consumption, routing or physical held-key tracking is performed.
 */
constexpr bool keyPressed(const InputEvent &event, InputKey key, InputModifiers modifiers) noexcept {
  const auto *value = std::get_if<KeyEvent>(&event);

  return keyPressed(event, key) && value->modifiers == modifiers;
}

/** @brief Match any listed semantic key on press/repeat, ignoring modifiers.
 * @param event Borrowed input event, never retained or consumed.
 * @param keys Key alternatives borrowed for this call; an empty list never matches.
 * @return True for PRESS/REPEAT of a listed key; false for RELEASE or non-key input.
 * The list requires no heap allocation; matching is linear in its size.
 * CHARACTER compares identity only, not its scalar. No routing state is introduced.
 */
constexpr bool keyPressed(const InputEvent &event, std::initializer_list<InputKey> keys) noexcept {
  const auto *value = std::get_if<KeyEvent>(&event);

  return value && value->action != KeyAction::RELEASE && std::ranges::contains(keys, value->key);
}
} // namespace portals
