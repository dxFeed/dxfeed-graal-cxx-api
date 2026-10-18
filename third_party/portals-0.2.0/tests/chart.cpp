// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <portals/canvas.hpp>
#include <portals/chart.hpp>
#include <portals/style.hpp>
#include <portals/virtual_table.hpp>
#include <set>

namespace allocation_test {
thread_local std::ptrdiff_t remaining{-1};
}

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *value = std::malloc(size ? size : 1)) {
    return value;
  }

  throw std::bad_alloc();
}

void operator delete(void *value) noexcept {
  std::free(value);
}

void operator delete(void *value, std::size_t) noexcept {
  std::free(value);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *value) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, std::size_t) noexcept {
  ::operator delete(value);
}

// Match the renderer's nothrow temporary allocations to the same fixture allocator.
void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new[](size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *value, const std::nothrow_t &) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, const std::nothrow_t &) noexcept {
  ::operator delete[](value);
}

namespace {
using namespace portals;
int failures{};

void check(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    std::cerr << "fixture failure\n";
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

struct Sink : PaintContext {
  std::vector<RenderCommand> commands;

  std::expected<void, TreeError> submit(std::span<const RenderCommand> values) override {
    for (const auto &value : values) {
      commands.push_back(value);
    }

    return {};
  }
};

CellStyle ink() {
  CellStyle value;
  value.background = RgbColor{10, 220, 30};

  return value;
}

std::set<std::pair<int, int>> pixels(const CellBuffer &buffer) {
  std::set<std::pair<int, int>> result;

  for (int y = 0; y < buffer.size().height; ++y) {
    for (int x = 0; x < buffer.size().width; ++x) {
      if (take(buffer.at({x, y})).style.background) {
        result.emplace(x, y);
      }
    }
  }

  return result;
}

void canvasOracle() {
  const auto clip = take(CellRect::from({1, 1}, {5, 4}));

  for (int x = -3; x < 9; ++x) {
    for (int y = -3; y < 9; ++y) {
      Sink sink;

      Canvas canvas(sink, clip);
      take(canvas.line({-2, 2}, {x, y}, ink()));
      std::set<std::pair<int, int>> expected;
      const int steps = std::max(std::abs(x + 2), std::abs(y - 2));

      for (int i = 0; i <= steps; ++i) {
        const double t = steps ? static_cast<double>(i) / steps : 0;
        const int px = static_cast<int>(std::lround(-2 + (x + 2) * t));
        const int py = static_cast<int>(std::lround(2 + (y - 2) * t));

        if (px >= 1 && px < 6 && py >= 1 && py < 5) {
          expected.emplace(px, py);
        }
      }

      auto frame = take(renderFullFrame({8, 7}, sink.commands));
      check(pixels(frame) == expected, "Canvas matches independent unbounded DDA then clipping oracle");
    }
  }

  Sink sink;

  Canvas canvas(sink, clip);
  take(canvas.line({std::numeric_limits<int>::min(), 2}, {std::numeric_limits<int>::max(), 2}, ink()));
  check(sink.commands.size() == 5, "huge offscreen line visits only visible dominant coordinates");
}

void bucket() {
  OhlcBucket value;
  check(!value.value(), "empty bucket has no candle");

  for (double tick : {10., 30., 2., 20.}) {
    check(value.add(tick), "finite tick accepted");
  }

  const auto bar = value.value();
  check(bar && bar->open == 10 && bar->high == 30 && bar->low == 2 && bar->close == 20,
        "bucket keeps first open, max high, min low and last close");
  check(!value.add(std::numeric_limits<double>::infinity()), "nonfinite tick rejected explicitly");
  check(value.value() && value.value()->close == 20, "rejected tick leaves domain aggregate unchanged");
  value.reset();
  check(!value.value(), "reset begins the next domain bucket");
}

struct Page : ModelSnapshot<Ohlc> {
  std::vector<ModelRow<Ohlc>> data;
  mutable std::size_t reads{};

  Page(std::uint64_t revision, std::vector<ModelRow<Ohlc>> rows, std::uint64_t total = 0)
      : ModelSnapshot(revision, total ? total : rows.size()), data(std::move(rows)) {
  }

  ModelRows<Ohlc> read(RowRange range) const override {
    reads += range.count;

    if (range.first > data.size() || range.count > data.size() - range.first) {
      return {};
    }

    return {RangeState::READY, std::span(data).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }
};

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
};

CellBuffer draw(Fixture &f, Node<Chart<Ohlc>> node, CellSize size) {
  std::optional<CellBuffer> result;
  take(f.tree->frame([&] {
    take(f.tree->arrange(f.root, take(CellRect::from({}, size)), take(CellRect::from({}, size))));
    take(f.tree->arrange(node, take(CellRect::from({}, size)), take(CellRect::from({}, size))));
    Sink sink;
    take(f.tree->paint(node, sink));
    result.emplace(take(renderFullFrame(size, sink.commands)));
  }));

  return std::move(*result);
}

void charts() {
  Fixture f;

  auto page = std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {1, 4, 0, 3}}, {{2, 1}, {3, 4, 1, 2}}});
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, page));
  ChartOptions options;
  options.mode = ChartMode::CANDLES;
  options.rising = ink();
  options.falling = ink();
  options.line = ink();
  options.y_range = ChartRange{0, 4};

  auto chart = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [](const Ohlc &v) {
      return std::optional{v};
    },
    options));

  const auto pin = take(model->pin());
  take(prepareChart(*f.tree, chart, pin, {5, 5}));

  auto rendered = draw(f, chart, {5, 5});
  check(take(rendered.at({0, 0})).glyph == "|" && take(rendered.at({0, 2})).glyph == "#",
        "candle distinguishes wick and open-close body");
  check(pixels(rendered) ==
          std::set<std::pair<int, int>>{{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 0}, {1, 1}, {1, 2}, {1, 3}},
        "candles render exact high-low columns");
  check(page->reads == 2, "chart reads only visible domain rows");
}

void themedChartBackground() {
  Fixture f;
  auto page = std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {1, 4, 0, 3}}});
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, page));
  ChartOptions options;

  options.mode = ChartMode::CANDLES;
  options.background.background = RgbColor{12, 13, 14};

  auto chart = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [](const Ohlc &value) {
      return std::optional{value};
    },
    options));
  StyleEngine styles(*f.tree);

  take(styles.bind(chart, {.type = "Chart"}));
  take(prepareChart(*f.tree, chart, take(model->pin()), {5, 5}));

  auto verify = [&](RgbColor expected) {
    const auto frame = draw(f, chart, {5, 5});

    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        check(take(frame.at({x, y})).style.background == expected,
              "chart blank cells, wick and body share the current themed background");
      }
    }

    check(take(frame.at({0, 2})).style.foreground == options.rising.foreground,
          "theme preserves semantic rising candle color");
  };

  verify({12, 13, 14});
  take(styles.replace(modernTheme()));
  verify(resolveStyle(modernTheme(), {.type = "Chart"}).normal.background.value());
  take(styles.replace(turboTheme()));
  verify(resolveStyle(turboTheme(), {.type = "Chart"}).normal.background.value());
  check(page->reads == 1, "theme replacement repaints without reading the provider again");

  options.rising.background = RgbColor{80, 20, 40};

  auto custom = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [](const Ohlc &value) {
      return std::optional{value};
    },
    options));

  take(styles.bind(custom, {.type = "Chart"}));
  take(prepareChart(*f.tree, custom, take(model->pin()), {5, 5}));

  const auto customFrame = draw(f, custom, {5, 5});

  check(take(customFrame.at({0, 2})).style.background == options.rising.background,
        "an explicit series background overrides the plot background");
}

void readableCandleGlyphs() {
  const auto unicode = take(detail::chartGlyphs({}, ChartGlyphSet::BOX_DRAWING));

  check(unicode.wick.bytes() == "\xe2\x94\x82" && unicode.body.bytes() == "\xe2\x96\x88" &&
          unicode.doji.bytes() == "\xe2\x94\x81",
        "Unicode candles use thin wicks, solid bodies and horizontal doji");

  const auto ascii = take(detail::chartGlyphs({}, ChartGlyphSet::ASCII));
  const auto wide = take(detail::chartGlyphs({.ambiguous = AmbiguousWidth::WIDE}, ChartGlyphSet::BOX_DRAWING));

  check(ascii.wick.bytes() == "|" && ascii.body.bytes() == "#", "ASCII candles retain portable brushes");
  check(wide.wick.bytes() == "|" && wide.body.bytes() == "#", "wide Unicode brushes fall back to one-cell ASCII");

  Fixture f;
  auto page = std::make_shared<Page>(
    1, std::vector<ModelRow<Ohlc>>{
         {{1, 1}, {1, 4, 0, 3}}, {{2, 1}, {3, 4, 0, 1}}, {{3, 1}, {2, 4, 0, 2}}, {{4, 1}, {100, 100, 100, 100}}});
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, page));
  ChartOptions options;

  options.mode = ChartMode::CANDLES;
  options.glyph_set = ChartGlyphSet::BOX_DRAWING;
  options.candle_gap = 1;

  auto chart = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [](const Ohlc &value) {
      return std::optional{value};
    },
    options));

  take(prepareChart(*f.tree, chart, take(model->pin()), {5, 5}));

  const auto frame = draw(f, chart, {5, 5});

  check(take(frame.at({0, 0})).glyph == unicode.wick.bytes() && take(frame.at({0, 2})).glyph == unicode.body.bytes() &&
          take(frame.at({4, 2})).glyph == unicode.doji.bytes(),
        "Unicode brushes reach retained candle painting, including doji");
  check(take(frame.at({2, 2})).style.foreground == options.falling.foreground,
        "falling candles retain their independent color");
  check(take(frame.at({1, 2})).glyph == " " && take(frame.at({3, 2})).glyph == " " && page->reads == 3,
        "candle gaps stay blank and reduce the visible consecutive domain range");
}

void candleSpacingBounds() {
  Fixture f;
  auto page = std::make_shared<Page>(
    1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {1, 4, 0, 3}}, {{2, 1}, {3, 4, 0, 1}}, {{3, 1}, {2, 4, 0, 2}}});
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, page));

  auto create = [&](std::int32_t gap, ChartMode mode = ChartMode::CANDLES) {
    ChartOptions options;

    options.mode = mode;
    options.candle_gap = gap;

    return take(f.tree->emplace<Chart<Ohlc>>(
      f.root, take(model->openRange()),
      [](const Ohlc &value) {
        return std::optional{value};
      },
      options));
  };

  const auto invalid = create(-1);
  const auto rejected = prepareChart(*f.tree, invalid, take(model->pin()), {3, 5});

  check(!rejected && rejected.error() == TreeError::INVALID_ARGUMENT && page->reads == 0,
        "negative candle spacing is rejected before provider access");

  const auto widest = create(std::numeric_limits<std::int32_t>::max());

  take(prepareChart(*f.tree, widest, take(model->pin()), {std::numeric_limits<std::int32_t>::max(), 1}));
  check(page->reads == 1, "maximum viewport and gap compute one visible candle without overflow");
  take(prepareChart(*f.tree, widest, take(model->pin()), {0, 5}));
  check(page->reads == 1, "zero-width chart does not read a domain bucket");

  const auto line = create(std::numeric_limits<std::int32_t>::max(), ChartMode::LINE);

  take(prepareChart(*f.tree, line, take(model->pin()), {3, 5}));
  take(f.tree->read(line, [&](const auto &view) {
    check(view.visibleRange() == RowRange{0, 3}, "line mode ignores candle spacing");
  }));
}
} // namespace

namespace {
void edgeCasesAndOwnership() {
  Fixture f;

  const auto nan = std::numeric_limits<double>::quiet_NaN();
  auto page = std::make_shared<Page>(1,
                                     std::vector<ModelRow<Ohlc>>{{{1, 1}, {0, 0, 0, 0}},
                                                                 {{2, 1}, {2, 2, 2, 2}},
                                                                 {{3, 1}, {nan, nan, nan, nan}},
                                                                 {{4, 1}, {4, 4, 4, 4}},
                                                                 {{5, 1}, {4, 4, 4, 4}}},
                                     1000000);

  auto model = take(Model<Ohlc>::create(*f.tree, f.root, page));
  ChartOptions options;
  options.line = ink();
  options.y_range = ChartRange{0, 4};
  int accesses{};
  bool throwing{};

  auto chart = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [&](const Ohlc &v) {
      ++accesses;

      if (throwing) {
        throw std::runtime_error("projection failure");
      }

      return std::optional{v};
    },
    options));

  auto pin = take(model->pin());
  take(prepareChart(*f.tree, chart, pin, {5, 5}));
  check(page->reads == 5 && accesses == 5, "million logical rows only read five visible buckets");

  auto frame = draw(f, chart, {5, 5});
  check(pixels(frame) == std::set<std::pair<int, int>>{{0, 4}, {1, 3}, {1, 2}, {3, 0}, {4, 0}},
        "line exact raster leaves NaN gap disconnected");
  check(page->reads == 5 && accesses == 5, "paint never reads model or invokes accessor");
  throwing = true;

  auto failure = prepareChart(*f.tree, chart, pin, {4, 4});
  check(!failure && failure.error() == TreeError::CALLBACK_FAILED, "projection exception is observable");
  check(pixels(draw(f, chart, {5, 5})) == pixels(frame), "failed prepare preserves old viewport and presentation");
  throwing = false;

  auto other_page = std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {9, 9, 9, 9}}});
  auto other = take(Model<Ohlc>::create(*f.tree, f.root, other_page));

  const auto before = accesses;
  failure = prepareChart(*f.tree, chart, take(other->pin()), {5, 5});
  check(!failure && failure.error() == TreeError::STALE_NODE && other_page->reads == 0 && before == accesses,
        "foreign pin fails before read/projection");
  ChartOptions automatic;
  automatic.line = ink();

  auto single_chart = take(f.tree->emplace<Chart<Ohlc>>(
    f.root, take(model->openRange()),
    [](const Ohlc &v) {
      return std::optional{v};
    },
    automatic));

  auto publish = [&](std::uint64_t revision, std::vector<ModelRow<Ohlc>> data) {
    take(model->publisher().publish(std::make_shared<Page>(revision, std::move(data))));
    take(f.scheduler->dispatch());
    take(prepareChart(*f.tree, single_chart, take(model->pin()), {2, 5}));
  };
  publish(2, {{{1, 1}, {3, 3, 3, 3}}});
  check(pixels(draw(f, single_chart, {2, 5})) == std::set<std::pair<int, int>>{{0, 2}},
        "single/degenerate sample centers vertically");
  publish(3, {});
  check(pixels(draw(f, single_chart, {2, 5})).empty(), "empty model renders blank");

  const auto extreme = std::numeric_limits<double>::max();
  publish(4, {{{1, 1}, {-extreme, -extreme, -extreme, -extreme}}, {{2, 1}, {extreme, extreme, extreme, extreme}}});
  frame = draw(f, single_chart, {2, 5});
  check(pixels(frame).contains({0, 4}) && pixels(frame).contains({1, 0}),
        "opposite extreme finite doubles normalize safely");
  take(prepareChart(*f.tree, chart, take(model->pin()), {2, 5}));
  frame = draw(f, chart, {2, 5});
  check(pixels(frame) == std::set<std::pair<int, int>>{{1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}},
        "finite segment crossing explicit range has exact clipped cells");
  const auto tiny = std::numeric_limits<double>::denorm_min();
  publish(5, {{{1, 1}, {-tiny, -tiny, -tiny, -tiny}}, {{2, 1}, {tiny, tiny, tiny, tiny}}});
  frame = draw(f, single_chart, {2, 5});
  check(pixels(frame).contains({0, 4}) && pixels(frame).contains({1, 0}), "subnormal scale retains both extrema");
  take(prepareChart(*f.tree, single_chart, take(model->pin()), {2, 0}));
  check(pixels(draw(f, single_chart, {2, 0})).empty(), "zero height paints no cells");
  take(f.tree->remove(single_chart));
  check(!prepareChart(*f.tree, single_chart, take(model->pin()), {2, 5}), "removed chart rejects stale handle");
}

// Application-owned forwarding, shared by the cadence and saturation examples.
std::expected<void, SchedulerError> forwardLatest(const Publisher<ModelSnapshot<Ohlc>> &publisher,
                                                  std::shared_ptr<const ModelSnapshot<Ohlc>> &pending) {
  if (!pending) {
    return {};
  }

  auto result = publisher.publish(pending);

  if (!result) {
    return std::unexpected(result.error());
  }

  pending.reset();

  return {};
}

void retainedLatestSurvivesSaturation() {
  for (bool superseded : {false, true}) {
    auto scheduler = take(Scheduler::create({.data_capacity = 1, .data_budget = 1}));
    auto tree = take(WidgetTree::create(*scheduler));
    auto root = take(tree->root());
    auto initial = std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {1, 1, 1, 1}}});
    auto model = take(Model<Ohlc>::create(*tree, root, initial));
    auto scope = take(scheduler->makeScope());
    auto filler = take(scheduler->connect<int>(scope, [](auto) {
    }));
    std::shared_ptr<const ModelSnapshot<Ohlc>> pending_latest;
    auto ingress = take(scheduler->connect<ModelSnapshot<Ohlc>>(
      scope,
      [&](auto snapshot) {
        pending_latest = std::move(snapshot);
      },
      SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST));
    OhlcBucket bucket;

    for (double tick : {10., 30., 2., 20.}) {
      check(bucket.add(tick), "every tick precedes saturated display ingress");
    }

    auto latest = std::make_shared<Page>(2, std::vector<ModelRow<Ohlc>>{{{1, 1}, *bucket.value()}});
    std::weak_ptr<const Page> latest_owner = latest;

    take(ingress.publisher.publish(latest));
    latest.reset();
    take(scheduler->dispatch());
    // Deterministically emulate another producer taking the freed queue slot before forwarding.
    take(filler.publisher.publish(std::make_shared<const int>(7)));

    auto refused = forwardLatest(model->publisher(), pending_latest);

    check(!refused && refused.error() == SchedulerError::WOULD_BLOCK, "saturated model queue rejects forwarding");
    check(pending_latest && !latest_owner.expired(), "failed forwarding retains the sole latest snapshot owner");
    take(scheduler->dispatch());

    if (superseded) {
      check(bucket.add(5), "new snapshot includes the next tick in the same domain bucket");

      auto newer = std::make_shared<Page>(3, std::vector<ModelRow<Ohlc>>{{{1, 1}, *bucket.value()}});
      std::weak_ptr<const Page> newer_owner = newer;

      take(ingress.publisher.publish(newer));
      newer.reset();
      take(scheduler->dispatch());
      check(latest_owner.expired(), "new aggregate replaces the one-slot pending display owner");
      take(filler.publisher.publish(std::make_shared<const int>(8)));
      refused = forwardLatest(model->publisher(), pending_latest);
      check(!refused && refused.error() == SchedulerError::WOULD_BLOCK && !newer_owner.expired(),
            "replacement also survives saturated forwarding");
      take(scheduler->dispatch());
    }

    // The producer publishes nothing more. One later caller cycle retries the retained owner.
    take(forwardLatest(model->publisher(), pending_latest));
    check(!pending_latest, "accepted forwarding clears the pending slot");
    take(scheduler->dispatch());

    const auto pin = take(model->pin());
    const auto range = take(pin.read({0, 1}));
    const auto &bar = range.rows()[0].value;

    check(pin.dataRevision() == (superseded ? 3U : 2U), "last revision arrives after saturation without republication");
    check(bar.open == 10 && bar.high == 30 && bar.low == 2 && bar.close == (superseded ? 5 : 20),
          "deferred latest snapshot preserves the complete OHLC aggregate");
    check(scheduler->metrics().data.high_water == 1, "saturation and retry stay within one scheduler slot");
  }
}

void samePinAndFrameCap() {
  constexpr SchedulerTime FRAME_INTERVAL = 100;
  constexpr SchedulerTime DURATION = 1000;

  for (const int ticks_per_ms : {1, 7}) {
    SchedulerTime now{};
    Fixture f;
    f.tree.reset();
    f.scheduler = take(Scheduler::create({}, SchedulerClock{[](void *p) noexcept {
                                                              return *static_cast<SchedulerTime *>(p);
                                                            },
                                                            &now}));
    f.tree = take(WidgetTree::create(*f.scheduler));
    f.root = take(f.tree->root());

    auto model = take(Model<Ohlc>::create(*f.tree, f.root, std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{})));
    std::uintptr_t chart_address{}, table_address{};

    auto chart = take(f.tree->emplace<Chart<Ohlc>>(f.root, take(model->openRange()), [&](const Ohlc &v) {
      chart_address = reinterpret_cast<std::uintptr_t>(&v);

      return std::optional{v};
    }));
    std::vector<TableColumn<Ohlc>> columns;
    columns.push_back({{1, 1}, take(GraphemeText::from("Close")), 8, [&](const Ohlc &v) {
                         table_address = reinterpret_cast<std::uintptr_t>(&v);

                         return take(GraphemeText::from(std::to_string(v.close)));
                       }});

    auto table = take(f.tree->emplace<VirtualTable<Ohlc>>(f.root, take(model->openRange()), std::move(columns)));
    auto scope = take(f.scheduler->makeScope());

    std::shared_ptr<const ModelSnapshot<Ohlc>> pending_latest;
    auto ingress = take(f.scheduler->connect<ModelSnapshot<Ohlc>>(
      scope,
      [&](auto snapshot) {
        pending_latest = std::move(snapshot);
      },
      SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST));
    OhlcBucket bucket;
    double oracle_open{}, oracle_high = -std::numeric_limits<double>::infinity(),
                          oracle_low = std::numeric_limits<double>::infinity(), oracle_close{};
    std::uint64_t revision = 1;
    std::size_t frames{};

    for (now = 1; now <= DURATION; ++now) {
      for (int tick = 0; tick < ticks_per_ms; ++tick) {
        const auto value = static_cast<double>((now * 17 + tick * 31) % 113) - 50;

        if (revision == 1) {
          oracle_open = value;
        }

        oracle_high = std::max(oracle_high, value);
        oracle_low = std::min(oracle_low, value);
        oracle_close = value;
        check(bucket.add(value), "finite replay tick accepted");
        take(ingress.publisher.publish(
          std::make_shared<Page>(++revision, std::vector<ModelRow<Ohlc>>{{{1, 1}, *bucket.value()}})));
      }
      // Frame cadence belongs to the caller; the injected clock avoids sleeps and real workers.

      if (now % FRAME_INTERVAL != 0) {
        continue;
      }

      take(f.scheduler->dispatch());
      take(forwardLatest(model->publisher(), pending_latest));
      take(f.scheduler->dispatch());

      const auto pin = take(model->pin());
      take(prepareChart(*f.tree, chart, pin, {8, 5}));
      take(prepareVirtualTable(*f.tree, table, pin, {8, 2}));
      check(chart_address == table_address && chart_address != 0, "table/chart access the exact same domain row");

      const auto rows = take(pin.read({0, 1}));
      const auto &bar = rows.rows()[0].value;
      check(bar.open == oracle_open && bar.high == oracle_high && bar.low == oracle_low && bar.close == oracle_close,
            "coalesced snapshot retains every tick's OHLC contribution");
      check(pin.dataRevision() == revision, "both views prepare latest coherent revision");
      take(f.tree->frame([&] {
        const auto bounds = take(CellRect::from({}, {8, 5}));
        take(f.tree->arrange(f.root, bounds, bounds));
        take(f.tree->arrange(chart, bounds, bounds));
        take(f.tree->arrange(table, take(CellRect::from({}, {8, 2})), bounds));

        Sink chart_sink;
        Sink table_sink;
        take(f.tree->paint(chart, chart_sink));
        take(f.tree->paint(table, table_sink));

        const auto chart_frame = take(renderFullFrame({8, 5}, chart_sink.commands));
        const auto table_frame = take(renderFullFrame({8, 2}, table_sink.commands));
        std::string close_cells;
        for (int column = 0; column < 8; ++column) {
          close_cells += take(table_frame.at({column, 1})).glyph;
        }

        check(take(chart_frame.at({0, 2})).glyph == "*", "chart paints the pinned close in the shared frame");
        check(close_cells.starts_with(std::to_string(bar.close).substr(0, 8)),
              "table paints the same pinned close in the shared frame");
      }));
      ++frames;
    }

    check(frames == DURATION / FRAME_INTERVAL, "higher update rate preserves exact caller frame cap");

    const auto metrics = f.scheduler->metrics();
    check(metrics.data.replaced > 0 && metrics.data.high_water <= 2,
          "replace-latest ingress is actually exercised and bounded");
  }
}

void prefetchedCompletionNavigation() {
  struct CachedPage : ModelSnapshot<Ohlc> {
    RowRange available;
    std::vector<ModelRow<Ohlc>> rows;

    CachedPage(std::uint64_t revision, std::uint64_t count, RowRange range)
        : ModelSnapshot(revision, count), available(range) {
      for (std::size_t i = 0; i < range.count; ++i) {
        const auto index = range.first + i;
        const auto price = static_cast<double>(index);

        rows.push_back({{index + 1, 1}, {price, price + 1, price - 1, price}});
      }
    }

    ModelRows<Ohlc> read(RowRange range) const override {
      if (range.first < available.first || range.first - available.first > available.count ||
          range.count > available.count - static_cast<std::size_t>(range.first - available.first)) {
        return {};
      }

      return {RangeState::READY,
              std::span(rows).subspan(static_cast<std::size_t>(range.first - available.first), range.count),
              {}};
    }
  };

  Fixture f;
  auto base = std::make_shared<CachedPage>(1, 1000, RowRange{});
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, base, ModelOptions{9, 2, 4}));
  auto consumer = take(model->openRange());
  const auto id = consumer.id();
  std::vector<double> observed;
  auto chart = take(f.tree->emplace<Chart<Ohlc>>(f.root, std::move(consumer), [&](const Ohlc &value) {
    observed.push_back(value.close);

    return std::optional{value};
  }));
  auto work = take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, 100));
  auto cached = std::make_shared<CachedPage>(1, 1000, RowRange{80, 60});
  const std::weak_ptr<CachedPage> lifetime = cached;

  take(work->complete(cached));
  take(f.scheduler->dispatch());
  work.reset();
  cached.reset();
  take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, 100));

  for (std::uint64_t first = 101; first <= 120; ++first) {
    check(!take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, first)),
          "chart forward navigation reuses its immutable completion");
  }

  for (std::uint64_t first = 120; first >= 90; --first) {
    observed.clear();
    check(!take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, first)),
          "chart backward navigation avoids replacement work");
    check(observed.size() == 3 && observed.front() == static_cast<double>(first),
          "chart cache samples match the current consecutive logical rows");
  }

  {
    const auto pin = take(model->pin());

    check(take(pin.range(id)).range() == RowRange{100, 3}, "chart consumer result remains exact");
    check(take(pin.read({90, 3})).state() == RangeState::PENDING, "chart reuse does not change public base reads");
  }

  auto oversized = prepareChart(*f.tree, chart, take(model->pin()), {10, 4}, 90);

  check(!oversized && oversized.error() == TreeError::INVALID_ARGUMENT, "chart cache obeys the model row budget");

  auto miss = take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, 200));

  check(miss.has_value(), "chart cache miss requests its exact new interval");
  take(f.tree->read(chart, [](const auto &value) {
    check(value.rangeState() == RangeState::PENDING, "chart does not display old samples at a new ordinal");
  }));
  take(miss->complete(std::make_shared<CachedPage>(1, 1000, RowRange{190, 60})));
  take(f.scheduler->dispatch());
  check(!take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, 90)) && miss->stopRequested(),
        "chart reverse after completion uses its ready retained page");
  miss.reset();
  take(model->publisher().publish(std::make_shared<CachedPage>(2, 1001, RowRange{})));
  take(f.scheduler->dispatch());

  auto revised = take(prepareChart(*f.tree, chart, take(model->pin()), {3, 4}, 90));

  check(revised && revised->dataRevision() == 2 && lifetime.expired(),
        "chart releases old completion storage and requests its new domain revision");
  take(f.tree->read(chart, [](const auto &value) {
    check(value.rangeState() == RangeState::PENDING && value.dataRevision() == 2,
          "chart never displays samples from a stale domain revision");
  }));
  check(!take(prepareChart(*f.tree, chart, take(model->pin()), {0, 0}, 90)),
        "zero chart viewport cancels loading without reading rows");
}

void pendingAndReentry() {
  Fixture f;

  auto pending = std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{}, 2);
  auto model = take(Model<Ohlc>::create(*f.tree, f.root, pending));

  auto chart = take(f.tree->emplace<Chart<Ohlc>>(f.root, take(model->openRange()), [](const Ohlc &v) {
    return std::optional{v};
  }));

  auto work = take(prepareChart(*f.tree, chart, take(model->pin()), {2, 4}));
  check(work.has_value(), "pending range returns caller-dispatched work");
  check(!take(prepareChart(*f.tree, chart, take(model->pin()), {2, 4})), "pending request returned only once");
  take(work->complete(
    std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {1, 1, 1, 1}}, {{2, 1}, {2, 2, 2, 2}}})));
  take(f.scheduler->dispatch());
  take(prepareChart(*f.tree, chart, take(model->pin()), {2, 4}));
  take(f.tree->read(chart, [](const auto &view) {
    check(view.rangeState() == RangeState::READY, "completion readiness reaches chart");
  }));
  check(pending->reads == 4, "completed consumer avoids new base read");
  take(model->publisher().publish(std::make_shared<Page>(2, std::vector<ModelRow<Ohlc>>{{{1, 1}, {3, 3, 3, 3}}})));
  take(f.scheduler->dispatch());
  Node<Chart<Ohlc>> reentrant;
  reentrant = take(f.tree->emplace<Chart<Ohlc>>(f.root, take(model->openRange()), [&](const Ohlc &v) {
    auto result = prepareChart(*f.tree, reentrant, take(model->pin()), {1, 3});
    check(!result && result.error() == TreeError::WOULD_BLOCK, "recursive prepare rejects without corrupting staging");

    return std::optional{v};
  }));
  take(prepareChart(*f.tree, reentrant, take(model->pin()), {1, 3}));
}

} // namespace

namespace {
void allocationFailuresPreservePresentation() {
  constexpr std::ptrdiff_t MAX_ALLOCATION_BUDGET = 256;
  bool completed{};
  std::ptrdiff_t rejected{};

  for (std::ptrdiff_t budget = 0; budget < MAX_ALLOCATION_BUDGET; ++budget) {
    Fixture f;
    auto initial =
      std::make_shared<Page>(1, std::vector<ModelRow<Ohlc>>{{{1, 1}, {2, 2, 2, 2}}, {{2, 1}, {4, 4, 4, 4}}});
    auto model = take(Model<Ohlc>::create(*f.tree, f.root, initial));
    ChartOptions options;

    options.line = ink();
    options.y_range = ChartRange{0, 10};

    auto chart = take(f.tree->emplace<Chart<Ohlc>>(
      f.root, take(model->openRange()),
      [](const Ohlc &bar) {
        return std::optional{bar};
      },
      options));

    take(prepareChart(*f.tree, chart, take(model->pin()), {2, 5}));

    const auto before = pixels(draw(f, chart, {2, 5}));
    auto replacement =
      std::make_shared<Page>(2, std::vector<ModelRow<Ohlc>>{{{1, 1}, {7, 7, 7, 7}}, {{2, 1}, {9, 9, 9, 9}}});

    take(model->publisher().publish(replacement));
    take(f.scheduler->dispatch());

    const auto pin = take(model->pin());

    take(f.tree->consumeFrameRequest());
    std::cerr << "chart allocation budget=" << budget << '\n';

    // Only preparation is fault-injected. Verification and teardown run with allocation restored.
    allocation_test::remaining = budget;

    auto result = prepareChart(*f.tree, chart, pin, {1, 3}, 1);

    allocation_test::remaining = -1;

    if (result) {
      completed = true;
      check(take(f.tree->consumeFrameRequest()) == Invalidation::PAINT, "successful prepare invalidates paint");
      take(f.tree->read(chart, [](const auto &view) {
        check(view.dataRevision() == 2 && view.visibleRange() == RowRange{1, 1},
              "successful staging commits replacement revision and interval together");
      }));

      break;
    }

    ++rejected;
    check(result.error() == TreeError::NO_MEMORY, "every rejected allocation is observable as NO_MEMORY");
    check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE, "failed staging preserves invalidation");
    take(f.tree->read(chart, [](const auto &view) {
      check(view.dataRevision() == 1 && view.visibleRange() == RowRange{0, 2} && view.rangeState() == RangeState::READY,
            "failed staging preserves prior frame revision, range and readiness");
    }));
    check(pixels(draw(f, chart, {2, 5})) == before, "failed staging preserves old viewport and exact cells");
  }

  check(completed && rejected > 0, "allocation sweep reaches a successful preparation after exercising failures");
}
} // namespace

int main() {
  prefetchedCompletionNavigation();
  allocationFailuresPreservePresentation();
  retainedLatestSurvivesSaturation();
  canvasOracle();
  bucket();
  charts();
  themedChartBackground();
  readableCandleGlyphs();
  candleSpacingBounds();
  edgeCasesAndOwnership();
  samePinAndFrameCap();
  pendingAndReentry();

  return failures ? 1 : 0;
}
