// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "composed_support.hpp"
#include <format>
#include <portals/table_column.hpp>
#include <portals/text_format.hpp>
#include <portals/ui.hpp>

using namespace composedConsumer;

namespace {
ManagedViewHandler<Ohlc> countPreparations(std::size_t &calls, Outcome &outcome) {
  return [&calls, &outcome](const ModelFrame<Ohlc> &frame,
                            const std::optional<ModelRequest<Ohlc>> &request) -> std::expected<void, TreeError> {
    check(frame.dataRevision() == DATA_REVISION && !request, "Resident model preparation requested work");
    ++calls;
    ++outcome.preparations;

    return {};
  };
}
} // namespace

int main() {
  ReplayTerminal terminal(CHECKPOINT_COUNT);
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  auto name = take(scene->state<std::string>("Portals"));
  auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  Nodes nodes;
  Outcome outcome;
  std::size_t listPreparations{};
  std::size_t tablePreparations{};
  std::size_t chartPreparations{};

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
  const auto format = [](const ModelRow<Ohlc> &row) {
    return text("Close " + std::to_string(static_cast<int>(row.value.close)));
  };
  const auto project = [](const Ohlc &value) {
    return std::optional{value};
  };

  ui::NodeRef<Label> title;
  ui::NodeRef<LineEdit> editor;
  ui::NodeRef<Button> button;
  ui::NodeRef<Badge> badge;
  ui::NodeRef<VirtualList<Ohlc>> list;
  ui::NodeRef<VirtualTable<Ohlc>> table;
  ui::NodeRef<Chart<Ohlc>> chart;

  const auto setup = [&](ManagedScene &, auto) -> std::expected<void, ManagedError> {
    ++outcome.setups;

    return {};
  };

  // Every source below expires before mount, including the table's column array.
  auto recipe = [&] {
    const auto price = cellFormat<double>("{:.0f}");
    const auto columns =
      take(tableColumns(tableColumn("Open", &Ohlc::open, price, 9), tableColumn("Close", &Ohlc::close, price, 9)));
    const std::string heading = "Composed prices: edit, run, inspect";
    const std::string caption = "Custom: Right, Delete";

    return ui::column(
      LinearOptions{.padding = {1, 1, 1, 1}}, ui::label(heading).reference(title),
      ui::lineEdit(name, {.result = edited}).reference(editor),
      ui::row(LinearOptions{.gap = 1}, ui::button("Run", clicked).reference(button),
              ui::custom<Badge>(caption).reference(badge).onEvent(customEvent)),
      ui::row(
        LinearOptions{.gap = 1},
        ui::virtualList(model, format,
                        ui::VirtualListConfig<Ohlc>{.policy = viewPolicy(LIST_WIDTH),
                                                    .prepared = countPreparations(listPreparations, outcome)})
          .reference(list)
          .setup(setup),
        ui::virtualTable(model, columns,
                         ui::VirtualTableConfig<Ohlc>{.policy = viewPolicy(TABLE_WIDTH),
                                                      .prepared = countPreparations(tablePreparations, outcome)})
          .reference(table)
          .setup(setup),
        ui::chart(model, project, ui::ChartConfig<Ohlc>{.prepared = countPreparations(chartPreparations, outcome)})
          .reference(chart)
          .setup(setup)));
  }();

  check(list.node() == Node<VirtualList<Ohlc>>{} && table.node() == Node<VirtualTable<Ohlc>>{} &&
          chart.node() == Node<Chart<Ohlc>>{},
        "Owning recipes published identities before mount");
  take(ui::mount(*scene, std::move(recipe)));

  nodes = {title.node(), editor.node(), button.node(), badge.node(), list.node(), table.node(), chart.node()};

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
  check(listPreparations > 0, "Named list lost its preparation callback");
  check(tablePreparations > 0, "Named table lost its preparation callback");
  check(chartPreparations > 0, "Named chart lost its preparation callback");
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
