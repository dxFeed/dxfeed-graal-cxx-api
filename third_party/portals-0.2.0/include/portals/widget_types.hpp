// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/geometry.hpp>
#include <portals/render.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>

namespace portals {
/** @brief Owning identity, independent of addresses; zero is never a live identity. */
struct NodeId {
  std::uint64_t session{};    ///< Process-unique tree session.
  std::size_t slot{};         ///< Registry position, not sibling order.
  std::uint64_t generation{}; ///< Reuse generation; exhausted slots retire.
  /** @brief Compare identity.
   * @param other Tree identity to compare.
   * @return True when all compared fields are equal.
   */
  bool operator==(const NodeId &other) const noexcept = default;
};
/** @brief Checked tree failures; rejected operations never start the requested callback. */
enum class TreeError {
  INVALID_ARGUMENT,   ///< An option, enum value or operation argument is invalid.
  WRONG_THREAD,       ///< Operation requires the tree's UI thread.
  WRONG_SESSION,      ///< An identity or scope belongs to another tree/scheduler.
  STALE_NODE,         ///< The observed node or model consumer is absent or retired.
  CLOSED,             ///< The owning tree or source is closed.
  WOULD_BLOCK,        ///< The current frame/reentry boundary or finite queue/work budget rejects this operation.
  NO_MEMORY,          ///< Allocation failed inside the operation boundary.
  IDENTITY_EXHAUSTED, ///< A session, generation or revision cannot advance without reuse.
  NODE_LIMIT,         ///< The configured live node, observer or consumer limit was reached.
  DEPTH_LIMIT,        ///< Inserting the child would exceed the configured tree depth.
  CALLBACK_FAILED,    ///< A user extension or callback threw.
  INVALID_GEOMETRY    ///< Required arranged geometry is absent or translation overflowed.
};
/** @brief Accumulated work; layout always includes painting. */
enum class Invalidation {
  NONE,  ///< No new work requested.
  PAINT, ///< Painting is required; arranged geometry remains usable.
  LAYOUT ///< Layout and painting are required; old arranged geometry becomes invalid.
};

/** @brief Owning arranged geometry; invalid geometry must never be used for hit-testing. */
struct NodeGeometry {
  CellRect local;   ///< Parent-relative rectangle.
  CellRect surface; ///< Explicitly translated surface rectangle.
  CellRect clip;    ///< Surface intersection with every ancestor and viewport.
  bool valid{};     ///< False before arrangement or after layout invalidation/failure.
};

/** @brief Finite storage and recursive traversal limits, including the root node. */
struct WidgetTreeOptions {
  std::size_t max_nodes{4096}; ///< Live and not-yet-reclaimed nodes share this budget.
  std::size_t max_depth{128};  ///< Root depth is one; values above 256 are rejected.
};

/** @brief Borrowed synchronous command sink supplied by the frame owner.
 * submit must consume commands immediately or retain their text only until the enclosing
 * WidgetTree::frame callback returns. The frame owner must render before leaving that
 * callback. No command/text/context borrow may survive the frame. No terminal I/O here.
 */
class PaintContext {
public:
  /** @brief Destroy the caller-owned sink. */
  virtual ~PaintContext() = default;
  /** @brief Borrow commands.
   * @param commands Commands and text borrowed until the enclosing frame callback returns; consume before then.
   * @return Success when accepted by the concrete sink, or its TreeError on failure.
   */
  virtual std::expected<void, TreeError> submit(std::span<const RenderCommand> commands) = 0;
};
} // namespace portals
