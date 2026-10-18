// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../../examples/replay/terminal.hpp"
#include <portals/table_column.hpp>
#include <portals/text_format.hpp>
#include <portals/ui.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>
#include <new>
#include <source_location>
#include <stdexcept>
#include <vector>

namespace {
thread_local bool counting{};
thread_local std::size_t allocations{};
} // namespace

// Count ordinary C++ allocation calls, not malloc, aligned allocation, or bytes.
void *operator new(std::size_t size) {
  if (auto *memory = std::malloc(std::max<std::size_t>(size, 1))) {
    allocations += counting ? 1 : 0;

    return memory;
  }

  throw std::bad_alloc();
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete(void *memory) noexcept {
  std::free(memory);
}

void operator delete[](void *memory) noexcept {
  std::free(memory);
}

void operator delete(void *memory, std::size_t) noexcept {
  std::free(memory);
}

void operator delete[](void *memory, std::size_t) noexcept {
  std::free(memory);
}

namespace {
using namespace portals;
using Clock = std::chrono::steady_clock;
constexpr std::size_t SAMPLES = 30;
constexpr std::size_t WARMUP = 3;
constexpr CellSize VIEWPORT{80, 24};

template <class T, class E>
T take(std::expected<T, E> result, std::source_location location = std::source_location::current()) {
  if (!result) {
    throw std::runtime_error(std::format("compact measurement API failure at line {}", location.line()));
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

struct Sample {
  std::uint64_t ns{};
  std::size_t allocations{};
};

template <class Action> Sample measure(Action action) {
  allocations = 0;
  counting = true;

  const auto begin = Clock::now();

  try {
    action();
  } catch (...) {
    counting = false;

    throw;
  }

  const auto end = Clock::now();

  counting = false;

  return {static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()),
          allocations};
}

std::array<Sample, 4> run(bool recipes) {
  example::ReplayTerminal terminal({}, 1, VIEWPORT);
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  Node<Column> root;
  std::array<Sample, 4> result;

  result[0] = measure([&] {
    if (recipes) {
      root = take(ui::mount(*scene, ui::column(ui::label("One"), ui::label("Two"), ui::label("Three"))));
    } else {
      root = take(scene->emplace<Column>(scene->root()));

      for (const auto caption : {"One", "Two", "Three"}) {
        take(scene->emplace<Label>(root, take(GraphemeText::from(caption))));
      }
    }

    take(scene->theme(modernTheme()));
  });
  result[1] = measure([&] {
    take(scene->prepare(*app, VIEWPORT));
    take(app->tree().frame([&] {
      take(
        layoutTree(app->tree(), app->root(), take(CellRect::from({}, VIEWPORT)), take(CellRect::from({}, VIEWPORT))));
    }));
  });
  result[2] = measure([&] {
    take(scene->prepare(*app, VIEWPORT));
  });
  result[3] = measure([&] {
    const auto stats = take(scene->run(terminal));

    if (stats.frames != 1 || !terminal.closed) {
      throw std::runtime_error("finite frame measurement changed behavior or cleanup");
    }
  });

  return result;
}

void report(bool recipes) {
  std::array<std::vector<Sample>, 4> samples;

  for (auto &phase : samples) {
    phase.reserve(SAMPLES);
  }

  for (std::size_t index = 0; index < WARMUP + SAMPLES; ++index) {
    const auto result = run(recipes);

    if (index >= WARMUP) {
      for (std::size_t phase = 0; phase < samples.size(); ++phase) {
        samples[phase].push_back(result[phase]);
      }
    }
  }

  constexpr std::array<std::string_view, 4> PHASES{"mount_and_theme", "initial_prepare_and_layout", "idle_prepare",
                                                   "one_frame_runner_and_cleanup"};

  std::cout << std::format("{{\"api\":\"{}\",\"phases\":[", recipes ? "descriptions" : "managed");

  for (std::size_t index = 0; index < samples.size(); ++index) {
    auto durations = samples[index];

    std::ranges::sort(durations, {}, &Sample::ns);

    const auto allocationBounds = std::ranges::minmax_element(samples[index], {}, &Sample::allocations);

    std::cout << std::format("{}{{\"phase\":\"{}\",\"p50_ns\":{},\"p95_ns\":{},"
                             "\"allocation_calls_min\":{},\"allocation_calls_max\":{}}}",
                             index ? "," : "", PHASES[index], durations[SAMPLES / 2 - 1].ns,
                             durations[(SAMPLES * 95 + 99) / 100 - 1].ns, allocationBounds.min->allocations,
                             allocationBounds.max->allocations);
  }

  std::cout << "]}";
}

void reportColumns(bool compact) {
  struct Price {
    double close;
  };

  constexpr std::size_t COLUMN_COUNT = 10;
  std::array<std::vector<Sample>, 2> samples;

  for (auto &phase : samples) {
    phase.reserve(SAMPLES);
  }

  for (std::size_t index = 0; index < WARMUP + SAMPLES; ++index) {
    std::vector<TableColumn<Price>> columns;

    const auto construction = measure([&] {
      columns.reserve(COLUMN_COUNT);

      for (std::size_t column = 0; column < COLUMN_COUNT; ++column) {
        if (compact) {
          columns.push_back(
            take(tableColumn({column + 1, 1}, "Close", &Price::close, cellFormat<double>("{:.2f}"), 9)));
        } else {
          columns.push_back(tableColumn<Price>({{column + 1, 1}, take(GraphemeText::from("Close")), 9}, &Price::close,
                                               [](double value) {
                                                 return GraphemeText::from(std::format("{:.2f}", value)).value();
                                               }));
        }
      }
    });
    const auto formatting = measure([&] {
      for (auto &column : columns) {
        if (column.format(Price{12.5}).bytes() != "12.50") {
          throw std::runtime_error("column measurement output differs");
        }
      }
    });

    if (index >= WARMUP) {
      samples[0].push_back(construction);
      samples[1].push_back(formatting);
    }
  }

  std::cout << std::format("{{\"api\":\"{}\",\"columns\":10,\"phases\":[",
                           compact ? "compact_columns" : "explicit_columns");

  constexpr std::array<std::string_view, 2> PHASES{"construction", "format_ten_cells"};

  for (std::size_t index = 0; index < samples.size(); ++index) {
    auto durations = samples[index];

    std::ranges::sort(durations, {}, &Sample::ns);

    const auto bounds = std::ranges::minmax_element(samples[index], {}, &Sample::allocations);

    std::cout << std::format("{}{{\"phase\":\"{}\",\"p50_ns\":{},\"p95_ns\":{},"
                             "\"allocation_calls_min\":{},\"allocation_calls_max\":{}}}",
                             index ? "," : "", PHASES[index], durations[SAMPLES / 2 - 1].ns,
                             durations[(SAMPLES * 95 + 99) / 100 - 1].ns, bounds.min->allocations,
                             bounds.max->allocations);
  }

  std::cout << "]}";
}
} // namespace

int main() {
  try {
    std::cout << "{\"samples\":30,\"warmup\":3,\"nodes\":4,\"cases\":[";
    report(false);
    std::cout << ',';
    report(true);
    std::cout << ',';
    reportColumns(false);
    std::cout << ',';
    reportColumns(true);
    std::cout << "]}\n";

    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }
}
