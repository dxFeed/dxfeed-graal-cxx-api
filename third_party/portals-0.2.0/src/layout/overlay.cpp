// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "grid_detail.hpp"
#include <new>

namespace portals {
namespace {
bool valid(OverlayOptions options) {
  return grid_detail::valid(options.padding) && grid_detail::valid(options.horizontal) &&
         grid_detail::valid(options.vertical);
}
} // namespace

Overlay::Overlay(OverlayOptions options, SizePolicy policy) : SizedWidget(policy), options_(options) {
}

OverlayOptions Overlay::options() const {
  return options_;
}

std::expected<CellSize, TreeError> Overlay::childAvailable(CellSize available) const {
  if (!valid(options_)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return grid_detail::inset(available, options_.padding);
}

std::expected<CellSize, TreeError> Overlay::measureChildren(std::span<const MeasuredChild> children) const {
  if (!valid(options_)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  grid_detail::Wide width{}, height{};

  for (const auto &child : children) {
    if (!grid_detail::valid(child)) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    width = std::max(width, grid_detail::requested(child.policy.width, child.desired.width));
    height = std::max(height, grid_detail::requested(child.policy.height, child.desired.height));
  }

  return grid_detail::size(width + options_.padding.left + options_.padding.right,
                           height + options_.padding.top + options_.padding.bottom);
}

std::expected<std::vector<CellRect>, TreeError> Overlay::arrangeChildren(std::span<const MeasuredChild> children,
                                                                         CellSize available) const {
  auto content = childAvailable(available);

  if (!content) {
    return std::unexpected(content.error());
  }

  auto slot = CellRect::from({options_.padding.left, options_.padding.top}, *content);

  if (!slot) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  try {
    std::vector<CellRect> result(0);

    if (children.size() > result.max_size()) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    result.reserve(children.size());

    for (const auto &child : children) {
      auto placed = grid_detail::place(child, *slot, options_.horizontal, options_.vertical);

      if (!placed) {
        return std::unexpected(placed.error());
      }

      result.push_back(*placed);
    }

    return std::expected<std::vector<CellRect>, TreeError>(std::in_place, result);
  } catch (const std::bad_alloc &) {
    return std::unexpected(TreeError::NO_MEMORY);
  }
}

std::expected<void, TreeError> setOverlayOptions(WidgetTree &tree, Node<Overlay> node, OverlayOptions options) {
  if (!valid(options)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  auto result = tree.edit(node, [&](Overlay &widget) {
    widget.options_ = options;
  });

  if (!result) {
    return result;
  }

  return tree.invalidate(node, Invalidation::LAYOUT);
}
} // namespace portals
