// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include "virtual_support.hpp"
#include <portals/managed.hpp>

namespace composedConsumer {
using namespace virtualConsumer;

inline constexpr std::size_t CHECKPOINT_COUNT = 13;
inline constexpr int LIST_WIDTH = 16;
inline constexpr int TABLE_WIDTH = 22;

inline StyleSheet palette(bool light) {
  return light ? themeFromTokens(
                   {{20, 30, 40}, {240, 245, 250}, {100, 160, 200}, {90, 100, 110}, {170, 200, 220}, {20, 70, 110}})
               : themeFromTokens(
                   {{220, 230, 240}, {20, 25, 35}, {40, 100, 150}, {120, 130, 140}, {40, 70, 100}, {100, 180, 220}});
}

inline SizePolicy viewPolicy(int width) {
  SizePolicy policy;

  policy.width = {.preferred = width, .mode = SizeMode::FIXED};

  return policy;
}

/** @brief Final application widget with owned text and no presentation binding. */
class Badge final : public Widget {
public:
  /** @param caption UTF-8 caption copied into the widget. */
  explicit Badge(std::string_view caption) : caption_(text(caption)) {
  }

  /** @param available Offered extent in terminal cells.
   * @return One row using the offered width.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return CellSize{available.width, 1};
  }

  /** @param local Allocated parent-relative cell rectangle.
   * @return Success after retaining local paint bounds, or invalid geometry.
   */
  std::expected<void, TreeError> arrange(CellRect local) override {
    auto bounds = CellRect::from({}, local.size());

    if (!bounds) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    bounds_ = *bounds;

    return {};
  }

  /** @param context Borrowed command sink for the current frame.
   * @return The original command submission result.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override {
    const std::array<RenderCommand, 2> commands{FillCommand{bounds_, bounds_, {}},
                                                TextCommand{{}, caption_, bounds_, {}}};

    return context.submit(commands);
  }

private:
  GraphemeText caption_;
  CellRect bounds_;
};

struct Nodes {
  Node<Label> title;
  Node<LineEdit> editor;
  Node<Button> button;
  Node<Badge> badge;
  Node<VirtualList<Ohlc>> list;
  Node<VirtualTable<Ohlc>> table;
  Node<Chart<Ohlc>> chart;
};

struct Outcome {
  int commits{};
  int clicks{};
  int customEvents{};
  int setups{};
  std::size_t preparations{};
  std::size_t idlePreparations{};
  std::size_t idleSubmissions{};
};

inline KeyEvent key(InputKey value) {
  return {value, {}, {}, KeyAction::PRESS};
}

// Input data only; mounting, state binding and scene callbacks remain in each consumer.
inline std::vector<InputEvent> script(std::size_t next) {
  switch (next) {
  case 0:
    return {KeyEvent{InputKey::CHARACTER, U'a', {.control = true}, KeyAction::PRESS}, PasteInputEvent{"Ada"}};
  case 1:
    return {key(InputKey::ENTER)};
  case 2:
    return {key(InputKey::TAB), key(InputKey::ENTER)};
  case 3:
    return {key(InputKey::RIGHT)};
  case 4:
    return {key(InputKey::DELETE_KEY)};
  case 5:
  case 6:
  case 7:
  case 8:
    return {key(InputKey::DOWN)};
  case 10:
    return {key(InputKey::F2)};
  default:
    return {};
  }
}

template <class T> inline void checkRetired(WidgetTree &tree, Node<T> node) {
  const auto result = tree.read(node, [](const T &) {
  });

  check(!result && result.error() == TreeError::STALE_NODE, "Self-removal left a live widget");
}

// Read-only assertions deliberately exercise the installed public widget APIs.
inline void inspect(std::size_t next, WidgetTree &tree, const Nodes &nodes, std::string_view name,
                    const Outcome &outcome) {
  take(tree.read(nodes.editor, [&](const LineEdit &editor) {
    if (next == 0) {
      check(name == "Portals" && editor.text().bytes() == "Portals" && !editor.dirty(), "Initial editor sync failed");
    } else if (next == 1) {
      check(name == "Portals" && editor.text().bytes() == "Ada" && editor.dirty(), "Draft committed too early");
    } else {
      check(name == "Ada" && editor.text().bytes() == "Ada" && !editor.dirty() && outcome.commits == 1,
            "Editor did not commit exactly once");
    }
  }));
  take(tree.read(nodes.list, [&](const VirtualList<Ohlc> &list) {
    check(list.rangeState() == RangeState::READY && list.visibleRange().count > 0, "List was not prepared");

    if (next >= 7) {
      check(list.selection() == RowId{2, 1}, "List navigation lost stable row identity");
    }
  }));
  take(tree.read(nodes.table, [&](const VirtualTable<Ohlc> &table) {
    check(table.rangeState() == RangeState::READY && table.visibleRange().count > 0, "Table was not prepared");

    if (next >= 9) {
      check(table.selection() == RowId{2, 1}, "Table navigation lost stable row identity");
    }
  }));
  take(tree.read(nodes.chart, [&](const Chart<Ohlc> &chart) {
    check(chart.rangeState() == RangeState::READY && chart.visibleRange().count > 0 &&
            chart.dataRevision() == DATA_REVISION,
          "Chart did not share the ready model revision");
  }));

  if (next >= 3) {
    check(outcome.clicks == 1, "Button did not invoke its callback exactly once");
    checkRetired(tree, nodes.button);
  }

  if (next >= 4) {
    check(outcome.customEvents == 1, "Final custom widget did not receive its event");
  }

  if (next >= 5) {
    checkRetired(tree, nodes.badge);
  }

  if (next >= 11) {
    take(tree.read(nodes.title, [](const Label &title) {
      check(title.computedStyle() && title.computedStyle()->normal.background == RgbColor{240, 245, 250},
            "Theme replacement did not reach the composed form");
    }));
  }
}

inline void report(const ReplayTerminal &terminal, const Outcome &outcome) {
  check(terminal.closed && !terminal.output.empty(), "Composed form did not render and close");
  check(outcome.commits == 1 && outcome.clicks == 1 && outcome.customEvents == 1 && outcome.setups == 3,
        "Composed form callback or setup count changed");
  std::cout << "composed form: name=Ada commits=1 clicks=1 custom=1 list=2 table=2 revision=7\n";

  constexpr std::string_view HEX = "0123456789abcdef";

  for (const unsigned char byte : terminal.output) {
    std::cout << HEX[byte >> 4] << HEX[byte & 15];
  }

  std::cout << '\n';
}
} // namespace composedConsumer
