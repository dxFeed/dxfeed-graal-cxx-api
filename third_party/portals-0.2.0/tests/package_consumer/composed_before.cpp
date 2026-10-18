// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "composed_support.hpp"

using namespace composedConsumer;

int main() {
  ReplayTerminal terminal(CHECKPOINT_COUNT);
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  auto name = take(scene->state<std::string>("Portals"));
  auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  Nodes nodes;
  Outcome outcome;

  const auto edited = [&](LineEditResult result) -> std::expected<void, TreeError> {
    if (result.error) {
      return std::unexpected(*result.error);
    }

    if (result.status == LineEditStatus::COMMITTED) {
      ++outcome.commits;
    }

    return {};
  };
  const auto clicked = [&]() -> std::expected<void, TreeError> {
    auto current = name.snapshot();

    if (!current) {
      return std::unexpected(current.error());
    }

    check((*current)->value == "Ada", "Button observed uncommitted domain state");
    ++outcome.clicks;

    return app->tree().remove(nodes.button);
  };
  const auto customEvent = [&](Badge &, const RoutedEvent &event) -> std::expected<EventReply, TreeError> {
    const auto *action = std::get_if<ActionEvent>(&event);

    if (!action) {
      return EventReply::IGNORED;
    }

    if (action->action == Action::ERASE_FORWARD) {
      auto removed = app->tree().remove(nodes.badge);

      if (!removed) {
        return std::unexpected(removed.error());
      }

      return EventReply::HANDLED;
    }

    if (action->action == Action::MOVE_RIGHT) {
      ++outcome.customEvents;

      return EventReply::HANDLED;
    }

    return EventReply::IGNORED;
  };
  const auto prepared = [&](const ModelFrame<Ohlc> &frame,
                            const std::optional<ModelRequest<Ohlc>> &request) -> std::expected<void, TreeError> {
    check(frame.dataRevision() == DATA_REVISION && !request, "Resident model preparation requested work");
    ++outcome.preparations;

    return {};
  };
  const auto format = [](const ModelRow<Ohlc> &row) {
    return text("Close " + std::to_string(static_cast<int>(row.value.close)));
  };
  const auto project = [](const Ohlc &value) {
    return std::optional{value};
  };
  const auto origin = []() -> std::expected<std::uint64_t, TreeError> {
    return 0;
  };

  const auto form = take(scene->emplace<Column>(scene->root(), LinearOptions{.padding = {1, 1, 1, 1}}));

  nodes.title = take(scene->emplace<Label>(form, text("Composed prices: edit, run, inspect")));
  nodes.editor = take(scene->emplace<LineEdit>(form));
  take(scene->bind(nodes.editor, name, edited));

  const auto actions = take(scene->emplace<Row>(form, LinearOptions{.gap = 1}));

  nodes.button = take(scene->emplace<Button>(actions, text("Run")));
  take(scene->onClick(nodes.button, clicked));
  nodes.badge = take(scene->emplace<Badge>(actions, "Custom: Right, Delete"));
  take(scene->onEvent(nodes.badge, customEvent));

  const auto views = take(scene->emplace<Row>(form, LinearOptions{.gap = 1}));
  const auto columns = priceColumns();

  nodes.list = take(scene->virtualList(views, model, format, {}, viewPolicy(LIST_WIDTH), prepared));
  ++outcome.setups;
  nodes.table = take(scene->virtualTable(views, model, std::span(columns), {}, viewPolicy(TABLE_WIDTH), prepared));
  ++outcome.setups;
  nodes.chart = take(scene->chart(views, model, project, {}, origin, prepared));
  ++outcome.setups;

  take(scene->theme(palette(false)));
  take(app->router().setFocus(nodes.editor));
  take(scene->onInput([&](const InputEvent &event) -> std::expected<bool, ManagedError> {
    const auto *pressed = std::get_if<KeyEvent>(&event);

    if (!pressed || pressed->key != InputKey::F2 || pressed->action == KeyAction::RELEASE) {
      return false;
    }

    auto changed = scene->theme(palette(true));

    if (!changed) {
      return std::unexpected(changed.error());
    }

    return true;
  }));

  terminal.checkpoint = [&](std::size_t next, TerminalRead &read) {
    inspect(next, app->tree(), nodes, take(name.snapshot())->value, outcome);

    if (next == 3) {
      take(app->router().setFocus(nodes.badge));
    } else if (next == 5) {
      take(app->router().setFocus(nodes.list));
    } else if (next == 7) {
      take(app->router().setFocus(nodes.table));
    } else if (next == 9) {
      read.resized = RESIZED_SIZE;
    } else if (next == 10) {
      const auto root = take(app->tree().inspect(app->root()));

      check(root.geometry.local.size() == RESIZED_SIZE, "Resize did not reach the composed layout");
    } else if (next == 11) {
      outcome.idlePreparations = outcome.preparations;
      outcome.idleSubmissions = terminal.submissions;
    } else if (next == 12) {
      check(outcome.preparations == outcome.idlePreparations && terminal.submissions == outcome.idleSubmissions,
            "Idle composed form prepared or submitted another frame");
    }

    read.events = script(next);
  };

  const auto stats = take(scene->run(terminal));

  check(stats.frames > 1 && outcome.preparations >= 3, "Composed form did not produce interactive frames");
  checkRetired(app->tree(), nodes.list);
  checkRetired(app->tree(), nodes.table);
  checkRetired(app->tree(), nodes.chart);
  check(take(name.snapshot())->value == "Ada", "Scene cleanup discarded domain state too early");

  const auto stopped = name.set("Late");

  check(!stopped && stopped.error() == TreeError::STALE_NODE, "Stopped scene accepted state writes");
  report(terminal, outcome);
  scene.reset();

  const auto expired = name.snapshot();

  check(!expired && expired.error() == TreeError::CLOSED, "State observer extended the scene lifetime");

  return EXIT_SUCCESS;
}
