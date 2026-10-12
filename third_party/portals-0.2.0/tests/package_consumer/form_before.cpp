// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "form_replay.hpp"

namespace {
using namespace consumer;

class Form final : public ApplicationScene {
public:
  explicit Form(Application &app) : app_(app), styles_(app.tree()) {
    auto &tree = app.tree();
    const auto column = take(tree.emplace<Column>(app.root()));

    nodes.title = take(tree.emplace<Label>(column, text("Name: Enter saves | F2 theme | F10 quit")));
    nodes.editor = take(tree.emplace<LineEdit>(column));
    nodes.alerts = take(tree.emplace<CheckBox>(column, text("Alerts")));
    nodes.run = take(tree.emplace<Button>(column, text("Run")));
    nodes.status = take(tree.emplace<Label>(column, text("Ready")));
    nodes.meter = take(tree.emplace<Meter>(column));
    name = take(Property<std::string>::create(tree, column, "Portals", Invalidation::PAINT));
    alerts = take(Property<bool>::create(tree, column, false, Invalidation::PAINT));
    level = take(Property<int>::create(tree, column, 50, Invalidation::PAINT));

    edit_ = take(bindLineEdit(
      tree, app.router(), nodes.editor, name,
      [this](LineEditResult result) {
        if (result.error) {
          record(std::unexpected(*result.error));

          return;
        }

        record(status(app_.tree(), nodes.status, editMessage(result, outcome)));
      },
      {},
      [](std::string_view value) {
        return !value.empty();
      }));
    checkbox_ = take(bindCheckbox(tree, app.router(), nodes.alerts, alerts, [this](CheckBoxResult result) {
      if (!result) {
        record(std::unexpected(result.error()));

        return;
      }

      ++outcome.checkboxActions;
      record(status(app_.tree(), nodes.status, "Alerts committed"));
    }));
    run_ = take(bindButton(app.router(), nodes.run, [this] {
      record(runAction());
    }));
    meter_ = take(app.router().bind(nodes.meter, [this](Meter &, const RoutedEvent &event) {
      auto result = meterAction(event);

      if (!result) {
        record(std::unexpected(result.error()));

        return EventReply::IGNORED;
      }

      return *result;
    }));

    for (auto node : {Node<Widget>(nodes.title), Node<Widget>(nodes.editor), Node<Widget>(nodes.alerts),
                      Node<Widget>(nodes.run), Node<Widget>(nodes.status)}) {
      take(styles_.bind(node, {.type = "Control"}));
    }

    take(styles_.bind(nodes.meter, {.type = "Meter"}));
    take(styles_.replace(palette(false)));
    take(app.router().setFocus(nodes.editor));
  }

  std::expected<void, TreeError> prepare(Application &app, CellSize) override {
    auto focused = app.router().focused();

    if (!focused) {
      return std::unexpected(focused.error());
    }

    auto edited = edit_->sync(*focused);
    auto checked = checkbox_->sync();

    if (!edited || !checked) {
      return !edited ? edited : checked;
    }

    for (auto node : {Node<Control>(nodes.title), Node<Control>(nodes.alerts), Node<Control>(nodes.run),
                      Node<Control>(nodes.status)}) {
      auto result = syncControl(app.tree(), node, *focused);

      if (!result) {
        return result;
      }
    }

    if (outcome.removedInsideCallback) {
      return {};
    }

    auto value = level->snapshot();

    if (!value) {
      return std::unexpected(value.error());
    }

    std::expected<bool, TreeError> changed{false};
    auto result = app.tree().edit(nodes.meter, [&](Meter &meter) {
      changed = meter.present((*value)->value, focused->id() == nodes.meter.id());
    });

    if (!result || !changed) {
      return !result ? result : std::unexpected(changed.error());
    }

    return *changed ? app.tree().invalidate(nodes.meter, Invalidation::PAINT) : std::expected<void, TreeError>{};
  }

  std::expected<bool, TreeError> input(Application &app, const InputEvent &event) override {
    if (pressed(event, InputKey::F10)) {
      app.requestStop();

      return true;
    }

    if (pressed(event, InputKey::F2)) {
      auto result = styles_.replace(palette(!light_));

      if (!result) {
        return std::unexpected(result.error().cause);
      }

      light_ = !light_;

      return true;
    }

    if (pressed(event, InputKey::F3)) {
      auto named = name->set("Remote");

      if (!named) {
        return std::unexpected(named.error());
      }

      auto changed = alerts->set(false);

      if (!changed) {
        return std::unexpected(changed.error());
      }

      outcome.notificationRejected = changed->rejected != 0;

      return true;
    }

    return false;
  }

  FormNodes nodes;
  Outcome outcome;
  std::shared_ptr<Property<std::string>> name;
  std::shared_ptr<Property<bool>> alerts;
  std::shared_ptr<Property<int>> level;
  std::optional<TreeError> callbackError;

private:
  void record(std::expected<void, TreeError> result) {
    if (!result) {
      callbackError = result.error();
      app_.requestStop();
    }
  }

  std::expected<void, TreeError> runAction() {
    auto named = name->snapshot();
    auto enabled = alerts->snapshot();

    if (!named || !enabled) {
      return std::unexpected(!named ? named.error() : enabled.error());
    }

    ++outcome.runActions;
    outcome.runName = (*named)->value;
    outcome.runAlerts = (*enabled)->value;

    return status(app_.tree(), nodes.status, "Run: " + outcome.runName + " / alerts=true");
  }

  std::expected<EventReply, TreeError> meterAction(const RoutedEvent &event) {
    const auto *action = std::get_if<ActionEvent>(&event);

    if (!action) {
      return EventReply::IGNORED;
    }

    if (action->action == Action::ERASE_FORWARD) {
      auto removed = app_.tree().remove(nodes.meter);

      if (!removed) {
        return std::unexpected(removed.error());
      }

      outcome.removedInsideCallback = true;

      return EventReply::HANDLED;
    }

    if (action->action != Action::MOVE_RIGHT && action->action != Action::MOVE_LEFT) {
      return EventReply::IGNORED;
    }

    auto current = level->snapshot();

    if (!current) {
      return std::unexpected(current.error());
    }

    const auto delta = action->action == Action::MOVE_RIGHT ? LEVEL_STEP : -LEVEL_STEP;
    auto changed = level->set(std::clamp((*current)->value + delta, MIN_LEVEL, MAX_LEVEL));

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return EventReply::HANDLED;
  }

  Application &app_;
  StyleEngine styles_;
  std::optional<LineEditBinding> edit_;
  std::optional<CheckBoxBinding> checkbox_;
  ActionBinding run_;
  ActionBinding meter_;
  bool light_{};
};
} // namespace

int main() {
  ReplayTerminal terminal;
  auto app = take(Application::create(formOptions(), terminal.clock()));
  Form form(*app);

  terminal.checkpoint = [&](std::size_t next) {
    const auto snapshot = inspect(app->tree(), form.nodes, take(form.name->snapshot())->value,
                                  take(form.alerts->snapshot())->value, take(form.level->snapshot())->value);

    verifyCheckpoint(next, snapshot, form.outcome);
  };

  const auto result = app->run(form, terminal);

  check(result.has_value() && !form.callbackError, "Low-level form run failed");
  reportCapture(terminal, form.outcome);

  return EXIT_SUCCESS;
}
