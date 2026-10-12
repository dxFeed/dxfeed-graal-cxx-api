// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/cell_buffer.hpp>

namespace portals {
/** @brief Owning resolved presentation; no actions or model state are encoded. */
struct ComputedStyle {
  CellStyle normal;        ///< Ordinary text and fill.
  CellStyle focused;       ///< Focused control text and fill.
  CellStyle disabled;      ///< Disabled control text and fill.
  CellStyle selected;      ///< Selected list/table row or editor range.
  CellStyle header;        ///< Fixed table heading.
  CellStyle caret;         ///< Editor software caret.
  std::int32_t minWidth{}; ///< Nonnegative intrinsic minimum in cells; zero leaves measurement unchanged.
  /** @param other Owning value to compare.
   * @return True when every paint value and metric agrees.
   */
  bool operator==(const ComputedStyle &other) const = default;
};
} // namespace portals
