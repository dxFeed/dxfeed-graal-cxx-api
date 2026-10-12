// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "virtual_scene.hpp"

namespace {
using namespace virtualConsumer;

// Including the snapshot adapters must not make legacy braced viewports ambiguous.
static_assert(requires(WidgetTree &tree, Node<VirtualList<Ohlc>> list, Node<VirtualTable<Ohlc>> table,
                       Node<Chart<Ohlc>> chart, const ModelFrame<Ohlc> &frame) {
  prepareVirtualList(tree, list, frame, {});
  prepareVirtualTable(tree, table, frame, {});
  prepareChart(tree, chart, frame, {});
});

void checkViews(Application &app, const Dashboard &dashboard, RangeState tableState, RangeState chartState,
                bool hidden = false) {
  take(app.tree().read(dashboard.table, [&](const VirtualTable<Ohlc> &view) {
    check(view.rangeState() == tableState, "Table readiness differs from its expected state");

    if (hidden) {
      check(view.visibleRange().count == 0, "Hidden table retained a visible range");
    } else {
      check(view.visibleRange().count == static_cast<std::size_t>(dashboard.tableViewport.height - 1),
            "Table preparation did not use its nested viewport");
    }
  }));
  take(app.tree().read(dashboard.chart, [&](const Chart<Ohlc> &view) {
    check(view.dataRevision() == DATA_REVISION, "Chart changed the pinned data revision");

    if (hidden) {
      check(view.visibleRange().count == 0, "Hidden chart retained a visible range");
    } else {
      check(view.rangeState() == chartState, "Chart readiness differs from its expected state");
      check(view.visibleRange().count == static_cast<std::size_t>(dashboard.chartViewport.width),
            "Chart preparation did not use its nested viewport");
    }
  }));
}

void checkPendingPair(const Dashboard &dashboard, std::size_t count) {
  check(dashboard.tableRequests.size() == count && dashboard.chartRequests.size() == count,
        "A preparation repeat lost or duplicated a provider request");
  check(dashboard.tableRequests.back().dataRevision() == DATA_REVISION &&
          dashboard.chartRequests.back().dataRevision() == DATA_REVISION,
        "Sibling requests do not use one data revision");
  check(!dashboard.tableRequests.back().stopRequested() && !dashboard.chartRequests.back().stopRequested(),
        "Current visible requests were cancelled");
}

// All three public snapshot overloads must compile and run against the relocated installation.
void checkListOverload() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  const auto root = take(tree->root());
  auto model = take(Model<Ohlc>::create(*tree, root, std::make_shared<const Prices>()));
  const auto list =
    take(tree->emplace<VirtualList<Ohlc>>(root, take(model->openRange()), [](const ModelRow<Ohlc> &row) {
      return text(std::to_string(static_cast<int>(row.value.close)));
    }));
  const auto frame = take(model->pin());
  const auto bounds = take(CellRect::from({}, {12, 4}));
  std::optional<LayoutSnapshot> snapshot;

  take(tree->frame([&] {
    take(layoutTree(*tree, root, bounds, bounds));
    snapshot = take(LayoutSnapshot::capture(*tree, root));
  }));
  check(!take(prepareVirtualList(*tree, list, frame, *snapshot)), "Ready list requested provider work");
  take(tree->read(list, [](const VirtualList<Ohlc> &view) {
    check(view.rangeState() == RangeState::READY && view.visibleRange().count == 4,
          "List snapshot overload ignored the arranged viewport");
  }));

  auto foreignTree = take(WidgetTree::create(*scheduler));
  const auto foreignRoot = take(foreignTree->root());

  take(foreignTree->setVisible(foreignRoot, false));

  const auto foreignLayout = take(LayoutSnapshot::capture(*foreignTree, foreignRoot));
  const auto wrongSession = prepareVirtualList(*tree, list, frame, foreignLayout);

  check(!wrongSession && wrongSession.error() == TreeError::WRONG_SESSION,
        "Snapshot identity error lost its original type");
  take(tree->remove(list));

  const auto removed = prepareVirtualList(*tree, list, frame, *snapshot);

  check(!removed && removed.error() == TreeError::STALE_NODE,
        "Captured geometry bypassed the live tree's removed-node check");
}

void preparationLimitCleanup() {
  ReplayTerminal terminal;
  ApplicationOptions options;

  options.maxPreparationPasses = 1;

  auto app = take(Application::create(options, terminal.clock()));
  ModelRequest<Ohlc> tableRequest;
  ModelRequest<Ohlc> chartRequest;

  {
    Dashboard dashboard(*app, true);
    const auto result = app->run(dashboard, terminal);

    check(!result && result.error().phase == ApplicationPhase::PREPARE &&
            std::get<TreeError>(result.error().cause) == TreeError::WOULD_BLOCK,
          "Pending view requests did not exhaust the configured preparation budget");
    check(dashboard.stopped && terminal.closed && terminal.submissions == 0 && terminal.output.empty(),
          "Failed preparation submitted a partial frame or skipped shutdown");
    checkPendingPair(dashboard, 1);

    // The scene owns model/provider lifetime; copies only observe cleanup after its destruction.
    tableRequest = dashboard.tableRequests.front();
    chartRequest = dashboard.chartRequests.front();
  }

  check(tableRequest.stopRequested() && chartRequest.stopRequested(),
        "Destroying the failed scene left pending provider receipts active");
}

void runLifecycle() {
  ReplayTerminal terminal(7);
  auto app = take(Application::create({}, terminal.clock()));
  Dashboard dashboard(*app, true);
  std::size_t settledPins{};
  std::size_t settledSubmissions{};
  std::size_t tableReadyPins{};
  std::uint64_t tableReadyRevision{};

  terminal.checkpoint = [&](std::size_t next, TerminalRead &read) {
    switch (next) {
    case 0:
      checkPendingPair(dashboard, 1);
      checkViews(*app, dashboard, RangeState::PENDING, RangeState::PENDING);
      check(dashboard.viewPasses > dashboard.pins, "Pending requests did not repeat layout preparation");
      check(dashboard.tableViewport == CellSize{30, 10} && dashboard.chartViewport == CellSize{28, 8},
            "Initial nested geometry is incorrect");
      read.resized = RESIZED_SIZE;

      break;
    case 1:
      checkPendingPair(dashboard, 2);
      check(dashboard.tableRequests.front().stopRequested() && dashboard.chartRequests.front().stopRequested(),
            "Resize did not cancel the superseded requests");
      check(dashboard.tableViewport == CellSize{39, 14} && dashboard.chartViewport == CellSize{36, 12},
            "Resize did not update nested viewports");
      take(app->tree().setVisible(dashboard.body, false));

      break;
    case 2: {
      check(dashboard.tableRequests.back().stopRequested() && dashboard.chartRequests.back().stopRequested(),
            "Hiding an ancestor did not cancel both requests");
      check(dashboard.tableViewport == CellSize{} && dashboard.chartViewport == CellSize{},
            "Hidden descendants did not receive zero viewports");
      checkViews(*app, dashboard, RangeState::PENDING, RangeState::PENDING, true);

      const auto ready = std::make_shared<const Prices>();
      const auto tableLate = dashboard.tableRequests.back().complete(ready);
      const auto chartLate = dashboard.chartRequests.back().complete(ready);

      check(!tableLate && tableLate.error() == SchedulerError::CLOSED && !chartLate &&
              chartLate.error() == SchedulerError::CLOSED,
            "Late completion of cancelled work was accepted");
      take(app->tree().setVisible(dashboard.body, true));

      break;
    }
    case 3: {
      checkPendingPair(dashboard, 3);
      checkViews(*app, dashboard, RangeState::PENDING, RangeState::PENDING);

      const auto ready = std::make_shared<const Prices>();

      // Completion only enqueues. Application performs dispatch before the next prepare/pin.
      take(dashboard.tableRequests.back().complete(ready));

      break;
    }
    case 4:
      checkViews(*app, dashboard, RangeState::READY, RangeState::PENDING);
      tableReadyPins = dashboard.pins;
      tableReadyRevision = dashboard.pinnedRevision;
      check(dashboard.pinned->dataRevision() == DATA_REVISION, "Table completion changed the data revision");
      take(dashboard.chartRequests.back().complete(std::make_shared<const Prices>()));

      break;
    case 5:
      checkViews(*app, dashboard, RangeState::READY, RangeState::READY);
      check(dashboard.pins > tableReadyPins && dashboard.pinnedRevision > tableReadyRevision,
            "Independent chart completion was not prepared in a later frame");
      check(dashboard.pinned->dataRevision() == DATA_REVISION, "Chart completion changed the data revision");
      settledPins = dashboard.pins;
      settledSubmissions = terminal.submissions;

      break;
    case 6:
      checkViews(*app, dashboard, RangeState::READY, RangeState::READY);
      check(dashboard.pins == settledPins && terminal.submissions == settledSubmissions,
            "Settled immutable views caused idle preparation or redraw");

      break;
    default:
      check(false, "Unexpected lifecycle checkpoint");
    }
  };

  const auto stats = take(app->run(dashboard, terminal));

  check(stats.frames == dashboard.pins, "A rendered frame did not have exactly one scene pin");
  check(terminal.closed, "Lifecycle transport was not closed");
  checkListOverload();
  preparationLimitCleanup();
  std::cout << "pending resize hide/show late completion shared pin idle list: verified\n";
}
} // namespace

int main() {
  runLifecycle();

  return EXIT_SUCCESS;
}
