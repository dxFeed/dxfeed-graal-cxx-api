// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/style.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <optional>
#include <tuple>

namespace portals {
namespace {
StyleError error(TreeError cause, std::string_view message) {
  return {cause, 1, 1, message};
}

bool matches(const StyleSelector &selector, const StyleTarget &target, StyleState state) {
  return (selector.type.empty() || selector.type == "*" || selector.type == target.type) &&
         (selector.id.empty() || selector.id == target.id) &&
         (selector.state == StyleState::ANY || selector.state == state) &&
         std::ranges::all_of(selector.classes, [&](const auto &name) {
           return std::ranges::find(target.classes, name) != target.classes.end();
         });
}

using Specificity = std::tuple<bool, std::size_t, bool>;

Specificity specificity(const StyleSelector &selector) {
  return {!selector.id.empty(), selector.classes.size() + (selector.state != StyleState::ANY),
          !selector.type.empty() && selector.type != "*"};
}

CellStyle resolvePaint(const StyleSheet &sheet, const StyleTarget &target, StyleState state, CellStyle base) {
  // Each declaration cascades independently: an absent high-priority value never erases a lower rule.
  std::array<std::optional<Specificity>, 5> ranks;
  const auto assign = [&]<class T>(const std::optional<T> &value, auto &destination, std::size_t index,
                                   Specificity rank) {
    if (value && (!ranks[index] || rank >= *ranks[index])) {
      destination = *value;
      ranks[index] = rank;
    }
  };

  for (const auto &rule : sheet.rules) {
    if (!matches(rule.selector, target, state)) {
      continue;
    }

    const auto rank = specificity(rule.selector);
    const auto &values = rule.declarations;

    assign(values.foreground, base.foreground, 0, rank);
    assign(values.background, base.background, 1, rank);
    assign(values.bold, base.bold, 2, rank);
    assign(values.italic, base.italic, 3, rank);
    assign(values.underline, base.underline, 4, rank);
  }

  return base;
}

std::expected<void, StyleError> validate(const StyleSheet &sheet) {
  if (sheet.rules.size() > MAX_STYLE_RULES) {
    return std::unexpected(error(TreeError::INVALID_ARGUMENT, "Too many style rules"));
  }

  for (const auto &rule : sheet.rules) {
    if (rule.selector.state < StyleState::ANY || rule.selector.state > StyleState::CARET ||
        (rule.declarations.minWidth && (*rule.declarations.minWidth < 0 || rule.selector.state != StyleState::ANY))) {
      return std::unexpected(error(TreeError::INVALID_ARGUMENT, "Invalid state or state-dependent metric"));
    }
  }

  return {};
}
} // namespace

ComputedStyle resolveStyle(const StyleSheet &sheet, const StyleTarget &target, ComputedStyle defaults) noexcept {
  defaults.normal = resolvePaint(sheet, target, StyleState::NORMAL, defaults.normal);
  defaults.focused = resolvePaint(sheet, target, StyleState::FOCUSED, defaults.focused);
  defaults.disabled = resolvePaint(sheet, target, StyleState::DISABLED, defaults.disabled);
  defaults.selected = resolvePaint(sheet, target, StyleState::SELECTED, defaults.selected);
  defaults.header = resolvePaint(sheet, target, StyleState::HEADER, defaults.header);
  defaults.caret = resolvePaint(sheet, target, StyleState::CARET, defaults.caret);
  std::optional<Specificity> rank;

  for (const auto &rule : sheet.rules) {
    if (rule.declarations.minWidth && matches(rule.selector, target, StyleState::ANY) &&
        (!rank || specificity(rule.selector) >= *rank)) {
      rank = specificity(rule.selector);
      defaults.minWidth = *rule.declarations.minWidth;
    }
  }

  return defaults;
}

StyleSheet themeFromTokens(ThemeTokens tokens) {
  StyleSheet sheet;
  StyleRule normal;

  normal.declarations.foreground = tokens.foreground;
  normal.declarations.background = tokens.background;
  sheet.rules.push_back(normal);

  const auto stateRule = [&](StyleState state, RgbColor foreground, RgbColor background, bool bold) {
    StyleRule rule;

    rule.selector.state = state;
    rule.declarations.foreground = foreground;
    rule.declarations.background = background;
    rule.declarations.bold = bold;
    sheet.rules.push_back(std::move(rule));
  };

  stateRule(StyleState::FOCUSED, tokens.background, tokens.accent, true);
  stateRule(StyleState::DISABLED, tokens.muted, tokens.background, false);
  stateRule(StyleState::SELECTED, tokens.foreground, tokens.selection, true);
  stateRule(StyleState::HEADER, tokens.heading, tokens.background, true);
  // Reverse the focused field palette so the caret remains visible over any glyph.
  stateRule(StyleState::CARET, tokens.accent, tokens.background, false);

  return sheet;
}

StyleSheet modernTheme() {
  return themeFromTokens(
    {{220, 225, 232}, {24, 28, 36}, {75, 180, 220}, {125, 130, 140}, {45, 75, 110}, {75, 180, 220}});
}

StyleSheet turboTheme() {
  return themeFromTokens({{255, 255, 85}, {0, 0, 170}, {0, 170, 170}, {170, 170, 170}, {170, 0, 0}, {255, 255, 255}});
}

// The following four palettes adapt Textual's MIT-licensed theme definitions.
// Copyright (c) 2021 Will McGugan. See vendor/textual/LICENSE and README.md for provenance.
StyleSheet nordTheme() {
  return themeFromTokens({.foreground = {0xD8, 0xDE, 0xE9},
                          .background = {0x2E, 0x34, 0x40},
                          .accent = {0x88, 0xC0, 0xD0},
                          .muted = {0x81, 0xA1, 0xC1},
                          .selection = {0x43, 0x4C, 0x5E},
                          .heading = {0xB4, 0x8E, 0xAD}});
}

StyleSheet gruvboxTheme() {
  return themeFromTokens({.foreground = {0xFB, 0xF1, 0xC7},
                          .background = {0x28, 0x28, 0x28},
                          .accent = {0x85, 0xA5, 0x98},
                          .muted = {0xA8, 0x9A, 0x85},
                          .selection = {0x50, 0x49, 0x45},
                          .heading = {0xFA, 0xBD, 0x2F}});
}

StyleSheet tokyoNightTheme() {
  return themeFromTokens({.foreground = {0xA9, 0xB1, 0xD6},
                          .background = {0x1A, 0x1B, 0x26},
                          .accent = {0xBB, 0x9A, 0xF7},
                          .muted = {0x7A, 0xA2, 0xF7},
                          .selection = {0x41, 0x48, 0x68},
                          .heading = {0xFF, 0x9E, 0x64}});
}

StyleSheet catppuccinLatteTheme() {
  return themeFromTokens({.foreground = {0x4C, 0x4F, 0x69},
                          .background = {0xEF, 0xF1, 0xF5},
                          .accent = {0x88, 0x39, 0xEF},
                          .muted = {0xDC, 0x8A, 0x78},
                          .selection = {0xCC, 0xD0, 0xDA},
                          .heading = {0xFE, 0x64, 0x0B}});
}

StyleEngine::StyleEngine(WidgetTree &tree, std::size_t maxBindings) : tree_(tree), maxBindings_(maxBindings) {
}

std::expected<void, StyleError> StyleEngine::checkBoundary() {
  auto root = tree_.root();

  if (!root) {
    return std::unexpected(error(root.error(), "Style tree is unavailable"));
  }

  auto access = tree_.access(root->id(), true);

  if (!access) {
    return std::unexpected(error(access.error(), "Style mutation requires an idle UI tree"));
  }

  return {};
}

std::expected<Invalidation, StyleError> StyleEngine::bind(Node<Widget> node, StyleTarget target) {
  auto boundary = checkBoundary();

  if (!boundary) {
    return std::unexpected(boundary.error());
  }

  auto access = tree_.access(node.id(), true);

  if (!access) {
    return std::unexpected(error(access.error(), "Style target is unavailable"));
  }

  try {
    auto bindings = bindings_;
    auto found = std::ranges::find(bindings, node, &Binding::node);

    if (found == bindings.end()) {
      // Prune stale observers before enforcing capacity; reuse cannot resurrect an old generation.
      std::erase_if(bindings, [&](const Binding &binding) {
        return !tree_.access(binding.node.id(), true);
      });

      if (bindings.size() >= maxBindings_) {
        return std::unexpected(error(TreeError::NODE_LIMIT, "Style binding budget exhausted"));
      }

      bindings.push_back({node, std::move(target)});
    } else {
      found->target = std::move(target);
    }

    return install(sheet_, std::move(bindings));
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(TreeError::NO_MEMORY, "Style allocation failed"));
  }
}

std::expected<Invalidation, StyleError> StyleEngine::bindBatch(std::span<const StyleRegistration> registrations) {
  auto boundary = checkBoundary();

  if (!boundary) {
    return std::unexpected(boundary.error());
  }

  bool identitiesValidated{};

  try {
    // Validate full identities before using slots as keys. Every admitted target belongs
    // to this tree's current generation; the temporary index owns no widget or metadata.
    std::map<std::size_t, std::optional<std::size_t>> positions;

    for (const auto &registration : registrations) {
      const auto node = registration.node;
      auto access = tree_.access(node.id(), true);

      if (!access) {
        return std::unexpected(error(access.error(), "Style target is unavailable"));
      }

      if (!positions.try_emplace(node.id().slot, std::nullopt).second) {
        return std::unexpected(error(TreeError::INVALID_ARGUMENT, "Duplicate target in style batch"));
      }
    }

    identitiesValidated = true;

    auto bindings = bindings_;

    std::erase_if(bindings, [&](const Binding &binding) {
      return !tree_.access(binding.node.id(), true);
    });

    // Index surviving bindings without sorting them: custom defaults keep their original
    // evaluation order. Ordered lookup bounds identity work even for adversarial slot keys.
    for (std::size_t index = 0; index < bindings.size(); ++index) {
      auto found = positions.find(bindings[index].node.id().slot);

      if (found != positions.end()) {
        found->second = index;
      }
    }

    for (const auto &registration : registrations) {
      const auto position = positions.find(registration.node.id().slot)->second;

      if (position) {
        bindings[*position].target = registration.target;

        continue;
      }

      if (bindings.size() >= maxBindings_) {
        return std::unexpected(error(TreeError::NODE_LIMIT, "Style binding budget exhausted"));
      }

      bindings.push_back({registration.node, registration.target});
    }

    return install(sheet_, std::move(bindings));
  } catch (const std::bad_alloc &) {
    // Argument errors preceded allocation in the original API. If building the index
    // fails, preserve that priority with an allocation-free scan; only this OOM path
    // pays quadratic duplicate detection. A completed validation needs no replay.
    if (!identitiesValidated) {
      for (std::size_t index = 0; index < registrations.size(); ++index) {
        const auto node = registrations[index].node;
        auto access = tree_.access(node.id(), true);

        if (!access) {
          return std::unexpected(error(access.error(), "Style target is unavailable"));
        }

        const auto preceding = registrations.first(index);

        if (std::ranges::find(preceding, node, &StyleRegistration::node) != preceding.end()) {
          return std::unexpected(error(TreeError::INVALID_ARGUMENT, "Duplicate target in style batch"));
        }
      }
    }

    return std::unexpected(error(TreeError::NO_MEMORY, "Style batch allocation failed"));
  }
}

std::expected<Invalidation, StyleError> StyleEngine::replace(StyleSheet sheet) {
  auto boundary = checkBoundary();

  if (!boundary) {
    return std::unexpected(boundary.error());
  }

  try {
    return install(std::move(sheet), std::vector<Binding>(bindings_));
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(TreeError::NO_MEMORY, "Style allocation failed"));
  }
}

std::expected<Invalidation, StyleError> StyleEngine::replacePss(std::string_view source) {
  auto boundary = checkBoundary();

  if (!boundary) {
    return std::unexpected(boundary.error());
  }

  auto sheet = parsePss(source);

  if (!sheet) {
    return std::unexpected(sheet.error());
  }

  try {
    return replace(*sheet);
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(TreeError::NO_MEMORY, "Style transfer allocation failed"));
  }
}

std::expected<Invalidation, StyleError> StyleEngine::refresh() {
  auto boundary = checkBoundary();

  if (!boundary) {
    return std::unexpected(boundary.error());
  }

  try {
    return install(sheet_, std::vector<Binding>(bindings_));
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(TreeError::NO_MEMORY, "Style allocation failed"));
  }
}

std::uint64_t StyleEngine::revision() const noexcept {
  return revision_;
}

std::expected<Invalidation, StyleError> StyleEngine::install(StyleSheet sheet, std::vector<Binding> &&bindings) {
  auto valid = validate(sheet);

  if (!valid) {
    return std::unexpected(valid.error());
  }

  if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
    return std::unexpected(error(TreeError::IDENTITY_EXHAUSTED, "Style revision exhausted"));
  }

  auto root = tree_.root();

  if (!root) {
    return std::unexpected(error(root.error(), "Style tree is unavailable"));
  }

  std::expected<Invalidation, StyleError> result = Invalidation::NONE;
  auto edited = tree_.edit(*root, [&](Widget &) {
    try {
      std::vector<ComputedStyle> staged(0);
      Invalidation invalidation = Invalidation::NONE;

      std::erase_if(bindings, [&](const Binding &binding) {
        return !tree_.access(binding.node.id(), true);
      });
      staged.reserve(bindings.size());

      {
        // Treat custom defaults as extension hooks: frame borrow blocks mutation and engine reentry.
        WidgetTree::Borrow frameGuard(tree_, true);

        for (const auto &binding : bindings) {
          auto widget = tree_.access(binding.node.id(), false);
          auto defaults = (*widget)->styleDefaults();
          auto next = resolveStyle(sheet, binding.target, defaults);

          if (next.minWidth < 0) {
            result = std::unexpected(error(TreeError::INVALID_ARGUMENT, "Widget default minimum width is negative"));

            return;
          }

          const auto previous = (*widget)->computedStyle_.value_or(defaults);

          if (previous.minWidth != next.minWidth) {
            invalidation = Invalidation::LAYOUT;
          } else if (previous != next && invalidation == Invalidation::NONE) {
            invalidation = Invalidation::PAINT;
          }

          staged.push_back(next);
        }
      }

      // No external code runs from here through commit. Revalidate before the sole invalidation.
      for (const auto &binding : bindings) {
        auto widget = tree_.access(binding.node.id(), true);

        if (!widget) {
          result = std::unexpected(error(widget.error(), "Style target changed during staging"));

          return;
        }
      }

      if (invalidation != Invalidation::NONE) {
        auto requested = tree_.invalidate(*root, invalidation);

        if (!requested) {
          result = std::unexpected(error(requested.error(), "Style invalidation rejected"));

          return;
        }
      }

      for (std::size_t index = 0; index < bindings.size(); ++index) {
        auto widget = tree_.access(bindings[index].node.id(), true);

        (*widget)->computedStyle_ = staged[index];
        (*widget)->styleRevision_ = revision_ + 1;
      }

      sheet_.rules.swap(sheet.rules);
      bindings_.swap(bindings);
      ++revision_;
      result = invalidation;
    } catch (const std::bad_alloc &) {
      result = std::unexpected(error(TreeError::NO_MEMORY, "Style staging allocation failed"));
    } catch (...) {
      result = std::unexpected(error(TreeError::CALLBACK_FAILED, "Widget style defaults threw"));
    }
  });

  if (!edited) {
    return std::unexpected(error(edited.error(), "Style transaction rejected"));
  }

  return result;
}
} // namespace portals
