// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_support.hpp"
#include <portals/ui.hpp>

namespace {
using namespace virtualConsumer;

bool causedBy(const ManagedError &error, TreeError cause) {
  const auto *value = std::get_if<TreeError>(&error.cause);

  return value && *value == cause;
}

auto describe(ManagedModel<Ohlc> model, ui::NodeRef<VirtualTable<Ohlc>> table, std::shared_ptr<int> lifetime) {
  auto columns = priceColumns();

  columns[0].format = [lifetime](const Ohlc &) {
    return text(std::to_string(*lifetime));
  };

  return ui::column(ui::virtualTable(model, columns).reference(table),
                    ui::virtualList(model,
                                    [](const ModelRow<Ohlc> &row) {
                                      return text(std::to_string(static_cast<int>(row.value.close)));
                                    }),
                    ui::chart(model, [](const Ohlc &value) {
                      return std::optional{value};
                    }));
}

void rollbackAndOwnership() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(), {.max_consumers = 3}));
  auto lifetime = std::make_shared<int>(123);
  const std::weak_ptr<int> observed = lifetime;
  ui::NodeRef<VirtualTable<Ohlc>> table;
  auto description = describe(model, table, lifetime);

  lifetime.reset();
  check(!observed.expired() && table.node() == Node<VirtualTable<Ohlc>>{}, "Recipe lost ownership or published early");

  const auto failed = ui::mount(
    *scene, ui::column(std::move(description),
                       ui::label("Fail").setup([](ManagedScene &, Node<Label>) -> std::expected<void, ManagedError> {
                         return std::unexpected(ManagedError{.phase = ManagedPhase::BIND,
                                                             .node = {},
                                                             .cause = TreeError::INVALID_GEOMETRY,
                                                             .application = {},
                                                             .cleanup = {}});
                       })));

  check(!failed && causedBy(failed.error(), TreeError::INVALID_GEOMETRY) && failed.error().phase == ManagedPhase::BIND,
        "Composition lost original typed setup error");
  check(table.node() == Node<VirtualTable<Ohlc>>{} && observed.expired(), "Rollback leaked refs or captured objects");
  check(take(app->tree().inspect(scene->root())).children.empty(), "Rollback retained its subtree");

  // Reusing all three range slots proves that rollback reclaimed model consumers too.
  const auto mounted = take(ui::mount(*scene, describe(model, table, std::make_shared<int>(123))));

  check(table.node() != Node<VirtualTable<Ohlc>>{}, "Successful mount did not publish ref");
  take(scene->remove(mounted));
  check(!app->tree().inspect(table.node()), "Removed ref was kept alive");
}

void invalidModelAndDuplicateBinding() {
  auto app = take(Application::create());
  auto scene = take(ManagedScene::create(*app));
  auto other = take(ManagedScene::create(*app));
  const auto model = take(other->model<Ohlc>(std::make_shared<const Prices>()));
  ui::NodeRef<Label> label;

  const auto failed = ui::mount(
    *scene, ui::column(ui::label("Earlier sibling").reference(label), ui::virtualTable(model, priceColumns())));

  check(!failed && causedBy(failed.error(), TreeError::INVALID_ARGUMENT), "Foreign model accepted");
  check(label.node() == Node<Label>{} && take(app->tree().inspect(scene->root())).children.empty(),
        "Foreign model failure did not roll back siblings");

  const auto local = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  const auto duplicate =
    ui::mount(*scene, ui::virtualTable(local, priceColumns())
                        .onEvent([](VirtualTable<Ohlc> &, const RoutedEvent &) -> std::expected<EventReply, TreeError> {
                          return EventReply::HANDLED;
                        }));

  check(!duplicate && causedBy(duplicate.error(), TreeError::INVALID_ARGUMENT), "Standard binding was overwritten");
  check(take(app->tree().inspect(scene->root())).children.empty(), "Duplicate binding leaked table");
}

void preparationOrderAndFailure() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  ui::NodeRef<VirtualTable<Ohlc>> table;
  ui::NodeRef<VirtualList<Ohlc>> list;
  ui::NodeRef<Chart<Ohlc>> chart;
  std::vector<int> order;
  auto lifetime = std::make_shared<int>(123);
  const std::weak_ptr<int> observed = lifetime;
  bool formatted{};
  bool originRead{};

  // This source array dies before mount. Its formatter must still be callable during the frame.
  auto recipe = [&] {
    auto columns = priceColumns();

    columns[0].format = [lifetime, &formatted](const Ohlc &) {
      formatted = true;

      return text(std::to_string(*lifetime));
    };

    return ui::row(
      ui::virtualTable(
        model, columns, {}, {},
        [&](const ModelFrame<Ohlc> &, const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
          order.push_back(1);

          return {};
        })
        .reference(table),
      ui::virtualList(
        model,
        [](const ModelRow<Ohlc> &) {
          return text("price");
        },
        {}, {},
        [&](const ModelFrame<Ohlc> &, const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
          order.push_back(2);

          return {};
        })
        .reference(list),
      ui::chart(
        model,
        [](const Ohlc &value) {
          return std::optional{value};
        },
        {},
        [&, lifetime]() -> std::expected<std::uint64_t, TreeError> {
          check(table.node() != Node<VirtualTable<Ohlc>>{} && list.node() != Node<VirtualList<Ohlc>>{},
                "Origin ran before refs were published");
          originRead = true;

          return 0;
        },
        [&, lifetime](const ModelFrame<Ohlc> &,
                      const std::optional<ModelRequest<Ohlc>> &) -> std::expected<void, TreeError> {
          order.push_back(3);
          take(app->tree().remove(chart.node()));
          check(!observed.expired(), "Self-removal destroyed an active callback capture");

          return std::unexpected(TreeError::INVALID_GEOMETRY);
        })
        .reference(chart));
  }();

  lifetime.reset();
  take(ui::mount(*scene, std::move(recipe)));

  const auto result = scene->run(terminal);

  check(!result && causedBy(result.error(), TreeError::INVALID_GEOMETRY) &&
          result.error().phase == ManagedPhase::PREPARE && result.error().node == chart.node().id(),
        "Declarative preparation lost typed cause, phase or node after self-removal");
  check(order == std::vector<int>{1, 2, 3} && originRead && formatted,
        "Composition reordered preparation or lost owned source columns");
  check(observed.expired() && terminal.closed, "Failed run did not release captures and close terminal");
}

void pendingViewsAndRejectedCompletion() {
  constexpr std::size_t CHECKPOINTS = 4;
  constexpr int TEXT_VIEW_WIDTH = 18;
  ReplayTerminal terminal(CHECKPOINTS);
  ApplicationOptions options;
  SizePolicy textViewPolicy;

  options.scheduler.data_capacity = 1;
  textViewPolicy.width = {.preferred = TEXT_VIEW_WIDTH, .mode = SizeMode::FIXED};

  auto app = take(Application::create(options, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(false)));
  std::array<std::optional<ModelRequest<Ohlc>>, 3> requests;
  ui::NodeRef<VirtualTable<Ohlc>> table;
  ui::NodeRef<VirtualList<Ohlc>> list;

  const auto handler = [&](std::size_t index) -> ManagedViewHandler<Ohlc> {
    return [&, index](const ModelFrame<Ohlc> &,
                      const std::optional<ModelRequest<Ohlc>> &request) -> std::expected<void, TreeError> {
      if (request) {
        requests[index] = request;
      }

      return {};
    };
  };

  const auto body = take(ui::mount(*scene, ui::row(
    ui::virtualTable(model, priceColumns(), {}, textViewPolicy, handler(0)).reference(table),
    ui::virtualList(model, [](const ModelRow<Ohlc> &) { return text("price"); }, {}, textViewPolicy, handler(1)).reference(list),
    ui::chart(model, [](const Ohlc &value) { return std::optional{value}; }, {}, {}, handler(2)))));

  terminal.checkpoint = [&](std::size_t next, TerminalRead &) {
    if (next == 0) {
      check(std::ranges::all_of(requests,
                                [](const auto &request) {
                                  return request && !request->stopRequested();
                                }),
            "Declarative provider callbacks did not receive all pending receipts");
      take(requests[0]->complete(std::make_shared<const Prices>()));

      const auto rejected = requests[1]->complete(std::make_shared<const Prices>());

      check(!rejected && rejected.error() == SchedulerError::WOULD_BLOCK,
            "Bounded queue did not reject the second completion");
    } else if (next == 1) {
      take(app->tree().read(table.node(), [](const auto &view) {
        check(view.rangeState() == RangeState::READY, "Accepted table completion was lost");
      }));
      take(requests[1]->complete(std::make_shared<const Prices>()));
    } else if (next == 2) {
      take(app->tree().read(list.node(), [](const auto &view) {
        check(view.rangeState() == RangeState::READY, "Rejected list completion did not recover on retry");
      }));
      take(app->tree().setVisible(body, false));
    } else {
      check(requests[2]->stopRequested(), "Hidden declarative chart retained pending work");

      const auto late = requests[2]->complete(std::make_shared<const Prices>());

      check(!late && late.error() == SchedulerError::CLOSED, "Hidden chart accepted a late completion");
    }
  };

  take(scene->run(terminal));
  check(terminal.closed, "Pending composition skipped terminal cleanup");
}

void missingProviderFailsDuringPreparation() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>(false)));
  const auto table = take(ui::mount(*scene, ui::virtualTable(model, priceColumns())));
  const auto result = scene->run(terminal);

  check(!result && causedBy(result.error(), TreeError::INVALID_ARGUMENT) &&
          result.error().phase == ManagedPhase::PREPARE && result.error().node == table.id(),
        "Missing provider did not preserve the managed preparation error");
  check(terminal.closed && !app->tree().inspect(table), "Missing provider skipped cleanup");
}
} // namespace

int main() {
  rollbackAndOwnership();
  invalidModelAndDuplicateBinding();
  preparationOrderAndFailure();
  pendingViewsAndRejectedCompletion();
  missingProviderFailsDuringPreparation();
  std::cout << "declarative virtual lifecycle checks passed\n";
}
