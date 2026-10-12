// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/dashboard/dashboard.hpp"
#include "../examples/replay/terminal.hpp"
#include "support/vt_oracle.hpp"
#include <functional>
#include <source_location>

namespace {
using namespace portals;
using example::take;

void check(bool condition, std::source_location where = std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("dashboard check line " + std::to_string(where.line()));
  }
}

void losslessAndSaturated(bool replay) {
  example::ReplayTerminal terminal({}, 1000);
  ApplicationOptions application;
  application.scheduler.data_capacity = application.scheduler.data_budget = 1;
  application.scheduler.control_capacity = application.scheduler.control_budget = 1;
  auto app = take(Application::create(application, terminal.clock()));
  dashboard::Options options;
  options.rate = 10000;
  options.replay = replay;
  std::size_t ingressSamples{};
  options.ingressObserver = [](void *context, SchedulerTime) {
    ++*static_cast<std::size_t *>(context);
  };
  options.ingressContext = &ingressSamples;

  dashboard::Scene scene(*app, options);

  const auto result = take(app->run(scene, terminal));
  check(terminal.closed && scene.tickCount() == 10000 && scene.scheduledTicks() == 10000);
  check(ingressSamples > 0 && scene.stoppedAt().has_value() && scene.pendingTicks() == 0);
  check(scene.coherent() && result.frames <= 31 && scene.displayedRevision() > 1);
  check(scene.maxPreparedRows() <= 10 + 4);
  double first{}, last{}, high = -1e9, low = 1e9;

  for (std::uint64_t i = 0; i != 10000; ++i) {
    const auto price =
      replay ? example::REPLAY_PRICES[i % example::REPLAY_PRICES.size()] : dashboard::syntheticPrice(i, options.seed);

    if (i == 0) {
      first = price;
    }

    last = price;
    high = std::max(high, price);
    low = std::min(low, price);
  }

  const auto aggregate = scene.aggregate();
  check(aggregate.open == first && aggregate.close == last && aggregate.high == high && aggregate.low == low);

  const auto metrics = app->scheduler().metrics();
  check(metrics.data.high_water == 1 && metrics.data.rejected > 0);
  test_support::VtOracle oracle(80, 24);
  oracle.feed(terminal.output);
  check(oracle.snapshot().cells[0].glyph == "P");
}

void pendingLatestOwnsRejectedValue() {
  auto scheduler = take(Scheduler::create({.data_capacity = 1, .data_budget = 1}));
  auto scope = take(scheduler->makeScope());
  auto occupied = take(scheduler->connect<int>(scope, [](auto) {
  }));
  std::uint64_t received{};
  auto destination = take(scheduler->connect<ModelSnapshot<dashboard::Quote>>(scope, [&](auto snapshot) {
    received = snapshot->revision();
  }));
  take(occupied.publisher.publish(std::make_shared<const int>(1)));

  std::shared_ptr<const ModelSnapshot<dashboard::Quote>> pending =
    std::make_shared<const dashboard::Snapshot>(9, 1, Ohlc{2, 5, 1, 3}, std::array<RowRange, 1>{{{0, 1}}});
  std::weak_ptr<const ModelSnapshot<dashboard::Quote>> observer = pending;
  auto rejected = dashboard::forwardLatest(destination.publisher, pending);
  check(!rejected && rejected.error() == SchedulerError::WOULD_BLOCK && pending && !observer.expired());
  take(scheduler->dispatch());
  take(dashboard::forwardLatest(destination.publisher, pending));
  check(!pending && !observer.expired());
  take(scheduler->dispatch());
  check(received == 9 && observer.expired());
}

class ObservedTerminal final : public example::ReplayTerminal {
public:
  using example::ReplayTerminal::ReplayTerminal;
  std::function<void()> beforeRead;

  TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
    if (beforeRead) {
      beforeRead();
    }

    return example::ReplayTerminal::read(deadline, stop);
  }
};

void delayedRangesAndKeyboard() {
  ObservedTerminal terminal({{100, "\x1b[B", {}},
                             {150, "\x1bOQ", {}},
                             {500,
                              "\t\x01"
                              "Edited note\r",
                              {}}},
                            900);
  ApplicationOptions application;
  dashboard::Scene *observed{};
  bool sawPending{}, sawReady{};
  std::uint64_t firstReady{};

  struct Context {
    dashboard::Scene **scene;
    example::ReplayTerminal *terminal;
    bool *pending;
    bool *ready;
    std::uint64_t *time;
  };

  Context context{&observed, &terminal, &sawPending, &sawReady, &firstReady};
  application.observer = {[](void *data, ApplicationPhase phase, std::chrono::nanoseconds) {
                            auto &context = *static_cast<Context *>(data);

                            if (phase != ApplicationPhase::PREPARE) {
                              return;
                            }

                            if ((*context.scene)->pendingVisible()) {
                              *context.pending = true;
                            } else if (!*context.ready) {
                              *context.ready = true;
                              const auto clock = context.terminal->clock();
                              *context.time = clock.read(clock.context);
                            }
                          },
                          &context};
  auto app = take(Application::create(application, terminal.clock()));
  dashboard::Options options;
  options.rate = 0;
  options.delayMs = 250;
  dashboard::Scene scene(*app, options);
  observed = &scene;
  take(setVirtualTableFirst(app->tree(), scene.table(), 10000));
  bool finalReady{};

  terminal.beforeRead = [&] {
    take(app->tree().read(scene.table(), [&](const auto &table) {
      finalReady =
        table.rangeState() == RangeState::READY && table.firstRow() >= 10000 && table.selection().has_value();
    }));
  };

  take(app->run(scene, terminal));
  check(sawPending && sawReady && firstReady >= 260);
  check(scene.coherent() && scene.turbo());
  check(take(scene.note().snapshot())->value == "Edited note");
  check(finalReady);
  check(!app->tree().inspect(scene.table()));
}

void continuousNavigationKeepsRowsVisible(bool backwards) {
  constexpr std::uint64_t FORWARD_FIRST = 240;
  constexpr std::uint64_t BACKWARD_FIRST = 300;
  const auto initialFirst = backwards ? BACKWARD_FIRST : FORWARD_FIRST;
  constexpr std::uint64_t INITIAL_PAGE_ROWS = 256;
  constexpr std::uint64_t REPEAT_START = 100;
  constexpr std::uint64_t REPEAT_INTERVAL = 10;
  constexpr std::uint64_t REPEAT_COUNT = 70;
  constexpr std::uint64_t RELEASE_AT = REPEAT_START + REPEAT_INTERVAL * REPEAT_COUNT;
  std::vector<example::ReplayTerminal::Input> input;

  for (std::uint64_t i = 0; i < REPEAT_COUNT; ++i) {
    input.push_back({REPEAT_START + i * REPEAT_INTERVAL, backwards ? "\x1b[A" : "\x1b[B", {}});
  }

  ObservedTerminal terminal(std::move(input), RELEASE_AT + 300);
  auto app = take(Application::create({}, terminal.clock()));
  dashboard::Options options;

  options.rate = 0;

  dashboard::Scene scene(*app, options);

  take(app->router().setFocus(scene.table()));
  take(setVirtualTableFirst(app->tree(), scene.table(), initialFirst));

  constexpr std::size_t SCREEN_WIDTH = 80;
  constexpr std::size_t TABLE_FIRST_ROW = 5;
  constexpr std::size_t CHART_FIRST_ROW = 14;
  constexpr std::size_t INSTRUMENT_WIDTH = 13;
  test_support::VtOracle oracle(SCREEN_WIDTH, 24);
  std::size_t consumed{}, heldSnapshots{}, blankSnapshots{}, blankCharts{};
  bool finalReady{};

  terminal.beforeRead = [&] {
    oracle.feed(std::string_view(terminal.output).substr(consumed));
    consumed = terminal.output.size();
    const auto clock = terminal.clock();
    const auto now = clock.read(clock.context);
    std::uint64_t first{};

    take(app->tree().read(scene.table(), [&](const auto &table) {
      first = table.visibleRange().first;
      finalReady =
        (backwards ? table.firstRow() < INITIAL_PAGE_ROWS : table.firstRow() > INITIAL_PAGE_ROWS) &&
        table.rangeState() == RangeState::READY &&
        table.selection() == RowId{backwards ? initialFirst - REPEAT_COUNT + 2 : initialFirst + REPEAT_COUNT, 1};
    }));

    if (now < REPEAT_START || now >= RELEASE_AT || first < INITIAL_PAGE_ROWS) {
      return;
    }

    ++heldSnapshots;
    const auto frame = oracle.snapshot();
    bool hasData{};

    // At 80x24 the chart follows nine table body rows, below four controls and a header.
    for (std::size_t y = TABLE_FIRST_ROW; y < CHART_FIRST_ROW; ++y) {
      for (std::size_t x = 0; x < INSTRUMENT_WIDTH; ++x) {
        const auto &glyph = frame.cells[y * SCREEN_WIDTH + x].glyph;

        hasData = hasData || (glyph.size() == 1 && glyph[0] >= '0' && glyph[0] <= '9');
      }
    }

    blankSnapshots += !hasData;
    bool hasCandle{};

    for (const auto &cell : frame.cells | std::views::drop(SCREEN_WIDTH * CHART_FIRST_ROW)) {
      hasCandle |= cell.glyph == "\xe2\x96\x88" || cell.glyph == "\xe2\x94\x82" || cell.glyph == "\xe2\x94\x81";
    }

    blankCharts += !hasCandle;
  };

  take(app->run(scene, terminal));
  check(heldSnapshots >= 5);
  check(finalReady);
  check(!app->tree().inspect(scene.table()));

  if (blankSnapshots) {
    throw std::runtime_error("continuous Down beyond initial page: " + std::to_string(blankSnapshots) + "/" +
                             std::to_string(heldSnapshots) + " held snapshots contain only loading placeholders");
  }

  if (blankCharts) {
    throw std::runtime_error("continuous Down: " + std::to_string(blankCharts) + "/" + std::to_string(heldSnapshots) +
                             " held snapshots have no chart candles");
  }
}

void noOpDiagnosticAction() {
  auto diagnostics = std::make_shared<Diagnostics>();
  take(diagnostics->enable({2048}));
  ApplicationOptions options;
  options.scheduler.diagnostics = diagnostics;
  example::ReplayTerminal terminal({{100, "\t\t\r", {}}}, 300);
  auto app = take(Application::create(options, terminal.clock()));
  dashboard::Options data;
  data.rate = 0;
  dashboard::Scene scene(*app, data);
  take(app->run(scene, terminal));

  const auto trace = diagnostics->snapshot();
  std::uint64_t unchanged{};

  for (const auto &record : trace.records) {
    if (record.reason == DiagnosticReason::VALUE_UNCHANGED) {
      unchanged = record.context.correlationId;
    }
  }

  check(unchanged != 0);
  check(std::ranges::any_of(trace.records, [&](const auto &record) {
    return record.context.correlationId == unchanged && record.reason == DiagnosticReason::FRAME_NOT_REQUESTED;
  }));
}

void clockJumpRemainsBounded() {
  ApplicationOptions application;
  application.inputSlice = std::chrono::milliseconds(5000);
  example::ReplayTerminal terminal({{5000, "\x1bOQ", {}}}, 5000);

  auto app = take(Application::create(application, terminal.clock()));
  dashboard::Options options;
  options.rate = 10000;
  options.maxTicksPerPump = 37;
  dashboard::Scene scene(*app, options);

  const auto result = take(app->run(scene, terminal));
  check(scene.scheduledTicks() == 50000 && scene.tickCount() == 37);
  check(scene.turbo() && result.inputs == 1 && result.iterations <= 3);
}

void chartAppearance(bool ascii, bool turbo, ColorMode colorMode = ColorMode::TRUE_COLOR) {
  std::vector<example::ReplayTerminal::Input> input;

  if (turbo) {
    input.push_back({100, "\x1bOQ", {}});
  }

  example::ReplayTerminal terminal(std::move(input), 500);
  ApplicationOptions application;

  application.writer.colorMode = colorMode;

  auto app = take(Application::create(application, terminal.clock()));
  dashboard::Options options;

  options.ascii = ascii;

  dashboard::Scene scene(*app, options);

  take(app->run(scene, terminal));

  test_support::VtOracle oracle(80, 24);

  oracle.feed(terminal.output);

  const auto frame = oracle.snapshot();
  const auto expectedBackground = turbo                            ? RgbColor{0, 0, 170}
                                  : colorMode == ColorMode::ANSI16 ? RgbColor{0, 0, 0}
                                                                   : RgbColor{24, 28, 36};
  bool sawBody{}, sawWick{};

  // The lower ten rows are the chart; inspect empty cells and both brushes,
  // not merely the first label of an otherwise successful application run.
  for (const auto &cell : frame.cells | std::views::drop(80 * 14)) {
    check(cell.style.background == expectedBackground);
    sawBody |= cell.glyph == (ascii ? "#" : "\xe2\x96\x88");
    sawWick |= cell.glyph == (ascii ? "|" : "\xe2\x94\x82");
  }

  check(sawBody && sawWick);
}
} // namespace

int main() {
  try {
    losslessAndSaturated(false);
    losslessAndSaturated(true);
    pendingLatestOwnsRejectedValue();
    delayedRangesAndKeyboard();
    continuousNavigationKeepsRowsVisible(false);
    continuousNavigationKeepsRowsVisible(true);
    clockJumpRemainsBounded();
    noOpDiagnosticAction();
    chartAppearance(false, false);
    chartAppearance(false, true);
    chartAppearance(true, false);
    chartAppearance(true, false, ColorMode::ANSI16);
    chartAppearance(false, true, ColorMode::ANSI16);
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }
}
