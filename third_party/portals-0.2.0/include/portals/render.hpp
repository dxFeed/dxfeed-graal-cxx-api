// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <portals/cell_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <variant>

namespace portals {

/**
 * @brief One horizontal text run; controls display placeholders, never line breaks.
 *
 * text is borrowed until renderFullFrame returns. Other fields own their values.
 * Keep the referenced text alive and unchanged while rendering. Commands have no
 * thread affinity; sharing them requires synchronization with text mutation.
 */
struct TextCommand {
  /** @brief First grapheme origin in surface cells; negative coordinates are valid. */
  CellPoint origin;
  /** @brief Validated text borrowed synchronously; no pointer is retained in the result. */
  const GraphemeText &text;
  /** @brief Half-open surface-cell clip; new graphemes must fit completely inside it. */
  CellRect clip;
  /** @brief Owning paint style for every visible grapheme. */
  CellStyle style{};
  /** @brief Ascending paint order; equal values preserve command input order. */
  std::int32_t z{};
};

/** @brief Fill an owning half-open area with styled one-cell ASCII spaces. */
struct FillCommand {
  /** @brief Area in surface cells, intersected with clip and frame bounds. */
  CellRect area;
  /** @brief Half-open surface-cell clip. */
  CellRect clip;
  /** @brief Owning paint style for every filled cell. */
  CellStyle style{};
  /** @brief Ascending paint order; equal values preserve command input order. */
  std::int32_t z{};
};

/** @brief One text or fill command, with TextCommand's synchronous borrow lifetime. */
using RenderCommand = std::variant<TextCommand, FillCommand>;

/** @brief Owning full-frame resource caps; over-limit scenes fail without truncation. */
struct RenderLimits {
  /** @brief Caps of the fresh owning result and its intern tables. */
  CellBufferLimits buffer{};
  /** @brief Maximum commands, including empty or offscreen commands. */
  std::size_t max_commands{65536};
  /** @brief Maximum summed original text bytes; repeated text runs count separately. */
  std::size_t max_text_bytes{4 * 1024 * 1024};
  /** @brief Maximum visited text graphemes plus clipped fill cells, not CPU instructions. */
  std::size_t max_work{4 * 1024 * 1024};
};

/** @brief Observable full-frame failure; no partial buffer is returned. */
enum class RenderErrorKind {
  /** @brief The command count exceeds its cap. */
  COMMAND_LIMIT,
  /** @brief Summed original text bytes exceed their cap. */
  TEXT_BYTE_LIMIT,
  /** @brief Visited graphemes plus clipped fill cells exceed their cap. */
  WORK_LIMIT,
  /** @brief A text run's complete captured width policy differs from the frame policy. */
  POLICY_MISMATCH,
  /** @brief Buffer creation or placement failed; RenderError::buffer_error describes it. */
  BUFFER_ERROR,
  /** @brief Temporary renderer storage could not be allocated or represented. */
  ALLOCATION_FAILURE
};

/** @brief Owning renderer error, independent of the scene lifetime. */
struct RenderError {
  /** @brief Failure category. */
  RenderErrorKind kind{};
  /** @brief Underlying error, meaningful only when kind is buffer_error. */
  CellBufferError buffer_error{};
  /** @brief Compare both owning error fields.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const RenderError &other) const noexcept = default;
};

/**
 * @brief Render a deterministic fresh frame, independent of diff/damage/output code.
 * @param size Owning cell extents; negative components normalize to zero.
 * @param commands Scene borrowed only during this synchronous call, with its text.
 * @param policy Complete owning width policy; every text run must match, even offscreen.
 * @param limits Owning scene/work/buffer caps; rejection never truncates a scene.
 * @return An independent buffer owning all display bytes/styles, or explicit failure.
 *
 * Any thread may call with stable input; there is no I/O, global state, callback or
 * terminal affinity. Concurrent calls on unchanged input are safe. The call never
 * mutates or invalidates inputs and retains no borrow. Buffer view invalidation follows
 * CellBuffer. Allocation failures become errors. The caller owns frame scheduling
 * and layout/paint invalidation, including width-policy changes.
 *
 * Text advances by whole measured graphemes using wide arithmetic. New glyphs are
 * placed only when their complete footprint fits clip and surface. Touching an old
 * wide continuation clears its lead as well, possibly outside the new clip, to
 * preserve complete clusters. Fill has the same atomic old-cluster cleanup rule.
 */
[[nodiscard]] std::expected<CellBuffer, RenderError> renderFullFrame(CellSize size,
                                                                     std::span<const RenderCommand> commands,
                                                                     WidthPolicy policy = {}, RenderLimits limits = {});

} // namespace portals
