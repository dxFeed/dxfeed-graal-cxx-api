// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "form_replay.hpp"
#include <portals/ui.hpp>

using namespace consumer;

int verifyTypedFailure() {
  ReplayTerminal terminal({{key(InputKey::ENTER)}});
  auto app = take(Application::create(formOptions(), terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto button = take(ui::mount(*scene, ui::button("Fail explicitly", []() -> std::expected<void, TreeError> {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  })));

  take(app->router().setFocus(button));

  const auto result = scene->run(terminal);

  check(!result, "Fallible business callback was silently ignored");

  const auto *cause = std::get_if<TreeError>(&result.error().cause);

  check(cause && *cause == TreeError::INVALID_GEOMETRY, "Callback lost its typed original cause");
  check(result.error().phase == ManagedPhase::INPUT && result.error().node == button.id(),
        "Callback error lost phase or node context");
  check(terminal.closed, "Callback error skipped terminal cleanup");

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--failure") {
    return verifyTypedFailure();
  }

  check(argc == 1, "Usage: portals_form_declarative [--failure]");

  ReplayTerminal terminal;
  auto app = take(Application::create(formOptions(), terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  FormNodes nodes;
  Outcome outcome;
  bool light{};

  auto name = take(scene->state<std::string>("Portals"));
  auto alerts = take(scene->state<bool>(false));
  auto level = take(scene->state<int>(50));

  const auto editAction = [&](LineEditResult result) -> std::expected<void, TreeError> {
    if (result.error) {
      return std::unexpected(*result.error);
    }

    return status(app->tree(), nodes.status, editMessage(result, outcome));
  };

  const auto alertsAction = [&](CheckBoxResult result) -> std::expected<void, TreeError> {
    if (!result) {
      return std::unexpected(result.error());
    }

    ++outcome.checkboxActions;

    return status(app->tree(), nodes.status, "Alerts committed");
  };

  const auto runAction = [&]() -> std::expected<void, TreeError> {
    auto named = name.snapshot();
    auto enabled = alerts.snapshot();

    if (!named || !enabled) {
      return std::unexpected(!named ? named.error() : enabled.error());
    }

    ++outcome.runActions;
    outcome.runName = (*named)->value;
    outcome.runAlerts = (*enabled)->value;

    return status(app->tree(), nodes.status, "Run: " + outcome.runName + " / alerts=true");
  };

  const auto meterAction = [&](Meter &, const RoutedEvent &event) -> std::expected<EventReply, TreeError> {
    const auto *action = std::get_if<ActionEvent>(&event);

    if (!action) {
      return EventReply::IGNORED;
    }

    if (action->action == Action::ERASE_FORWARD) {
      auto removed = app->tree().remove(nodes.meter);

      if (!removed) {
        return std::unexpected(removed.error());
      }

      outcome.removedInsideCallback = true;

      return EventReply::HANDLED;
    }

    if (action->action != Action::MOVE_RIGHT && action->action != Action::MOVE_LEFT) {
      return EventReply::IGNORED;
    }

    auto current = level.snapshot();

    if (!current) {
      return std::unexpected(current.error());
    }

    const auto delta = action->action == Action::MOVE_RIGHT ? LEVEL_STEP : -LEVEL_STEP;
    auto changed = level.set(std::clamp((*current)->value + delta, MIN_LEVEL, MAX_LEVEL));

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return EventReply::HANDLED;
  };

  const auto presentMeter = [&](Meter &meter, Node<Widget> focused) -> std::expected<void, TreeError> {
    auto current = level.snapshot();

    if (!current) {
      return std::unexpected(current.error());
    }

    auto changed = meter.present((*current)->value, focused.id() == nodes.meter.id());

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return *changed ? app->tree().invalidate(nodes.meter, Invalidation::PAINT) : std::expected<void, TreeError>{};
  };

  const auto nonemptyName = [](std::string_view value) {
    return !value.empty();
  };

  // References observe the completed tree; no identity is exposed by a partial mount.
  ui::NodeRef<Label> title, statusLabel;
  ui::NodeRef<LineEdit> editor;
  ui::NodeRef<CheckBox> alertsBox;
  ui::NodeRef<Button> run;
  ui::NodeRef<Meter> meter;

  take(ui::mount(
    *scene,
    ui::column(
      ui::label("Name: Enter saves | F2 theme | F10 quit").reference(title),

      ui::lineEdit(name, {.result = editAction, .validate = nonemptyName}).reference(editor),

      ui::checkbox("Alerts", alerts, alertsAction).reference(alertsBox),

      ui::button("Run", runAction).reference(run),

      ui::label("Ready").reference(statusLabel),

      ui::custom<Meter>().reference(meter).style({.type = "Meter"}).onEvent(meterAction).onPrepare(presentMeter))));

  // Business callbacks start in run(), after all observing identities are available.
  nodes = {title.node(), editor.node(), alertsBox.node(), run.node(), statusLabel.node(), meter.node()};

  take(scene->theme(palette(false)));
  take(app->router().setFocus(nodes.editor));

  take(scene->onInput([&](const InputEvent &event) -> std::expected<bool, ManagedError> {
    if (pressed(event, InputKey::F10)) {
      app->requestStop();

      return true;
    }

    if (pressed(event, InputKey::F2)) {
      auto changed = scene->theme(palette(!light));

      if (!changed) {
        return std::unexpected(changed.error());
      }

      light = !light;

      return true;
    }

    if (pressed(event, InputKey::F3)) {
      auto named = name.set("Remote");

      if (!named) {
        return std::unexpected(
          ManagedError{.phase = ManagedPhase::INPUT, .node = nodes.editor.id(), .cause = named.error()});
      }

      auto changed = alerts.set(false);

      if (!changed) {
        return std::unexpected(
          ManagedError{.phase = ManagedPhase::INPUT, .node = nodes.alerts.id(), .cause = changed.error()});
      }

      outcome.notificationRejected = changed->rejected != 0;

      return true;
    }

    return false;
  }));

  terminal.checkpoint = [&](std::size_t next) {
    const auto snapshot = inspect(app->tree(), nodes, take(name.snapshot())->value, take(alerts.snapshot())->value,
                                  take(level.snapshot())->value);

    verifyCheckpoint(next, snapshot, outcome);
  };

  const auto result = scene->run(terminal);

  check(result.has_value(), "Managed form run failed");
  check(take(name.snapshot())->value == "Remote", "State owner disappeared before scene destruction");

  const auto stoppedWrite = name.set("Too late");

  check(!stoppedWrite && stoppedWrite.error() == TreeError::STALE_NODE, "Stopped state accepted a new write");
  reportCapture(terminal, outcome);

  scene.reset();

  const auto expired = name.snapshot();

  check(!expired && expired.error() == TreeError::CLOSED, "Observing state handle extended owner lifetime");

  return EXIT_SUCCESS;
}
