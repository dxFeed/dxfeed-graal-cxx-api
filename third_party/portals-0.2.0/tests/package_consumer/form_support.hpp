// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/application.hpp>
#include <portals/line_edit.hpp>
#include <portals/style.hpp>

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

namespace consumer {
using namespace portals;

inline constexpr int LEVEL_STEP = 5;
inline constexpr int MIN_LEVEL = 0;
inline constexpr int MAX_LEVEL = 100;

inline void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

// Setup and test assertions terminate on failure; application callbacks return their errors.
template <class T, class E> T take(std::expected<T, E> result) {
  check(result.has_value(), "Consumer setup or assertion failed");

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

inline GraphemeText text(std::string_view value) {
  return take(GraphemeText::from(value));
}

inline StyleSheet palette(bool light) {
  return light ? themeFromTokens(
                   {{20, 30, 40}, {240, 245, 250}, {100, 160, 200}, {90, 100, 110}, {170, 200, 220}, {20, 70, 110}})
               : themeFromTokens(
                   {{220, 230, 240}, {20, 25, 35}, {40, 100, 150}, {120, 130, 140}, {40, 70, 100}, {100, 180, 220}});
}

inline bool pressed(const InputEvent &event, InputKey key) {
  const auto *value = std::get_if<KeyEvent>(&event);

  return value && value->action != KeyAction::RELEASE && value->key == key;
}

struct Outcome {
  int commits{};
  int validations{};
  int conflicts{};
  int cancellations{};
  int invalidText{};
  int checkboxActions{};
  int runActions{};
  bool notificationRejected{};
  bool removedInsideCallback{};
  std::string runName;
  bool runAlerts{};
};

inline std::string editMessage(const LineEditResult &result, Outcome &outcome) {
  switch (result.status) {
  case LineEditStatus::COMMITTED:
    ++outcome.commits;
    return "Name committed";
  case LineEditStatus::VALIDATION_FAILED:
    ++outcome.validations;
    return "Name must not be empty";
  case LineEditStatus::CONFLICT:
    ++outcome.conflicts;
    return "Name changed externally";
  case LineEditStatus::CANCELLED:
    ++outcome.cancellations;
    return "Draft reset";
  case LineEditStatus::INVALID_TEXT:
    ++outcome.invalidText;
    return "Invalid text rejected";
  default:
    return "Editing";
  }
}

inline std::expected<void, TreeError> status(WidgetTree &tree, Node<Label> node, std::string_view message) {
  auto value = GraphemeText::from(message);

  if (!value) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return setControlText(tree, node, std::move(*value));
}

/** @brief The same final extension type is used by both complete consumers. */
class Meter final : public Widget {
public:
  /** @param level Committed domain level in percent.
   * @param focused Current focus presentation supplied by the owning scene.
   * @return Whether the retained presentation changed, or a text error category.
   */
  std::expected<bool, TreeError> present(int level, bool focused) {
    if (initialized_ && level_ == level && focused_ == focused) {
      return false;
    }

    auto label = GraphemeText::from("Level " + std::to_string(level));

    if (!label) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    text_ = std::move(*label);
    level_ = level;
    focused_ = focused;
    initialized_ = true;

    return true;
  }

  /** @param available Offered extent in cells.
   * @return One row using the offered width.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return CellSize{available.width, 1};
  }

  /** @param local Allocated rectangle in parent coordinates.
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

  /** @param context Synchronous borrowed command sink.
   * @return The sink's exact result; text remains owned through the frame.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override {
    const auto styles = computedStyle().value_or(ComputedStyle{});
    const auto appearance = focused_ ? styles.focused : styles.normal;
    const std::array<RenderCommand, 2> commands{FillCommand{bounds_, bounds_, appearance},
                                                TextCommand{{}, text_, bounds_, appearance}};

    return context.submit(commands);
  }

private:
  GraphemeText text_ = text("");
  CellRect bounds_;
  int level_{};
  bool focused_{};
  bool initialized_{};
};

struct FormNodes {
  Node<Label> title;
  Node<LineEdit> editor;
  Node<CheckBox> alerts;
  Node<Button> run;
  Node<Label> status;
  Node<Meter> meter;
};

struct Snapshot {
  std::string name;
  bool alerts{};
  int level{};
  std::string draft;
  bool dirty{};
  bool checked{};
  bool meterAlive{};
  bool light{};
};

// This is test-only observation: no registration, mutation, synchronization or focus recovery.
inline Snapshot inspect(WidgetTree &tree, const FormNodes &nodes, std::string name, bool alerts, int level) {
  Snapshot result{std::move(name), alerts, level};

  take(tree.read(nodes.editor, [&](const LineEdit &editor) {
    result.draft = editor.text().bytes();
    result.dirty = editor.dirty();
  }));
  take(tree.read(nodes.alerts, [&](const CheckBox &checkbox) {
    result.checked = checkbox.checked();
  }));
  take(tree.read(nodes.title, [&](const Label &title) {
    check(title.computedStyle().has_value(), "Standard control has no computed style");
    result.light = title.computedStyle()->normal.background == RgbColor{240, 245, 250};
  }));

  auto meter = tree.read(nodes.meter, [](const Meter &widget) {
    check(widget.computedStyle().has_value(), "Custom widget has no computed style");
  });

  result.meterAlive = meter.has_value();

  if (!meter) {
    check(meter.error() == TreeError::STALE_NODE, "Custom widget failed for an unexpected reason");
  }

  return result;
}

inline void verifyCheckpoint(std::size_t next, const Snapshot &value, const Outcome &outcome) {
  switch (next) {
  case 0:
    check(value.name == "Portals" && value.draft == "Portals" && !value.dirty, "Initial sync failed");
    break;
  case 1:
    check(value.name == "Portals" && value.draft == "Ada" && value.dirty, "Draft committed before Enter");
    break;
  case 2:
    check(value.name == "Ada" && !value.dirty && outcome.commits == 1, "Enter did not commit once");
    break;
  case 3:
    check(value.alerts && value.checked && outcome.checkboxActions == 1, "Checkbox two-way state failed");
    break;
  case 4:
    check(outcome.runActions == 1 && outcome.runName == "Ada" && outcome.runAlerts, "Run lost committed state");
    break;
  case 5:
    check(value.level == 55 && value.meterAlive, "Custom input did not change state");
    break;
  case 6:
    check(value.light, "F2 did not replace the palette");
    break;
  case 7:
    check(value.name == "Ada" && value.draft.empty() && value.dirty && outcome.validations == 1,
          "Rejected validation lost the draft or changed state");
    break;
  case 8:
    check(value.draft == "Ada" && !value.dirty && outcome.cancellations == 1, "Escape did not reset the draft");
    break;
  case 9:
    check(value.draft == "Ada!" && value.dirty, "Conflict setup did not retain a dirty draft");
    break;
  case 10:
    check(value.name == "Remote" && value.draft == "Ada!" && value.dirty, "External update overwrote the draft");
    check(!value.alerts && !value.checked && outcome.notificationRejected && outcome.checkboxActions == 1,
          "Rejected notification did not resync or repeated the business callback");
    break;
  case 11:
    check(value.name == "Remote" && value.draft == "Ada!" && outcome.conflicts == 1, "Conflict was not preserved");
    break;
  case 12:
    check(value.draft == "Remote" && !value.dirty && outcome.cancellations == 2, "Escape did not adopt latest state");
    break;
  case 13:
    check(value.draft == "Remote" && !value.dirty && outcome.invalidText == 1, "Invalid UTF-8 changed the draft");
    break;
  case 14:
    check(!value.meterAlive && outcome.removedInsideCallback, "Self-removal did not complete safely");
    break;
  default:
    check(false, "Unexpected replay checkpoint");
  }
}
} // namespace consumer
