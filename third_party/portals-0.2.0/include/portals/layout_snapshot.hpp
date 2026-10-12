// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/widget.hpp>

#include <memory>
#include <vector>

namespace portals {
/** @brief Owning immutable identities and local sizes from one completed layout pass.
 * Copies share immutable value storage, without widgets, tree owners or borrowed text.
 * Captured values remain readable after tree invalidation, removal or destruction.
 * Concurrent const reads are safe; assignment requires caller synchronization.
 */
class LayoutSnapshot {
public:
  /** @brief Copy every live node in a subtree, including effectively hidden descendants.
   * Call on the tree's UI thread after successful layout, preferably inside its frame.
   * No layout, callbacks, mutation or frame request consumption is performed.
   * @param tree Live UI tree borrowed only during this call.
   * @param node Checked root of the subtree to capture; ancestor visibility is respected.
   * @return Owning snapshot, a tree access error, NO_MEMORY, or INVALID_GEOMETRY when
   * an effectively visible node has no valid arranged geometry. Hidden nodes need no
   * valid geometry and receive zero size; disabled nodes retain their arranged size.
   */
  [[nodiscard]] static std::expected<LayoutSnapshot, TreeError> capture(WidgetTree &tree, Node<Widget> node);
  /** @brief Read a captured local extent without consulting the current tree.
   * @param node Observer identifying the captured session, slot and generation.
   * @return Captured width/height in cells, zero for an effectively hidden node,
   * WRONG_SESSION for another tree, or STALE_NODE for an empty/absent identity or
   * a moved-from snapshot. Removal after capture does not invalidate captured identities.
   * @note No allocation, tree affinity, mutation or exceptions; clipping does not reduce this extent.
   */
  [[nodiscard]] std::expected<CellSize, TreeError> viewport(Node<Widget> node) const noexcept;

private:
  struct Record {
    NodeId id;
    CellSize size;
  };

  LayoutSnapshot(std::uint64_t session, std::shared_ptr<const std::vector<Record>> records);
  std::uint64_t session_;
  std::shared_ptr<const std::vector<Record>> records_;
};
} // namespace portals
