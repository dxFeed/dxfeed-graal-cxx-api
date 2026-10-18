// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/ui.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace {
using namespace portals;
using Clock = std::chrono::steady_clock;

enum class Phase : std::size_t {
  CONSTRUCTION,
  INITIAL_STYLE_REGISTER,
  INITIAL_PREPARE,
  DIRTY_PREPARE,
  THEME_SWITCH_PREPARE,
  REMOVE_ROWS,
  CLEANUP,
  COUNT
};

constexpr std::size_t API_COUNT = 3;
constexpr std::size_t PHASE_COUNT = std::to_underlying(Phase::COUNT);
constexpr std::size_t NODES_PER_ROW = 3;
constexpr std::size_t MAX_ROWS = 1024;
constexpr std::size_t MAX_SAMPLES = 200;
constexpr std::size_t MAX_WARMUP = 50;
constexpr CellSize VIEWPORT{80, 24};
constexpr std::string_view LABEL_TEXT = "Benchmark row";
constexpr std::string_view BUTTON_TEXT = "Activate";
constexpr std::array<std::string_view, PHASE_COUNT> PHASE_NAMES{
  "construction", "initial_style_register", "initial_prepare", "dirty_prepare", "theme_switch_prepare", "remove_rows",
  "cleanup"};

enum class Api {
  LOW_LEVEL,
  MANAGED,
  DESCRIPTIONS
};

struct ApiDescriptor {
  Api api;
  std::string_view name;
};

constexpr std::array<ApiDescriptor, API_COUNT> APIS{ApiDescriptor{Api::LOW_LEVEL, "low_level"},
                                                    ApiDescriptor{Api::MANAGED, "managed"},
                                                    ApiDescriptor{Api::DESCRIPTIONS, "descriptions"}};

struct Options {
  std::vector<std::size_t> sizes{32, 128, 512};
  std::size_t samples{30};
  std::size_t warmup{5};
  std::string output;
  bool checkOnly{};
};

void check(bool condition, std::string_view message, std::source_location where = std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(std::format("{} at line {}", message, where.line()));
  }
}

template <class T, class E>
T take(std::expected<T, E> result, std::source_location where = std::source_location::current()) {
  check(result.has_value(), "unexpected API failure", where);

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

std::size_t number(std::string_view value, std::size_t maximum) {
  std::size_t result{};
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);

  check(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && result <= maximum,
        "invalid numeric option");

  return result;
}

Options parse(int argc, char **argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string_view key(argv[i]);

    if (key == "--check") {
      options.checkOnly = true;

      continue;
    }

    check(i + 1 < argc, "missing option value");

    const std::string_view value(argv[++i]);

    if (key == "--samples") {
      options.samples = number(value, MAX_SAMPLES);
      check(options.samples > 0, "samples must be positive");
    } else if (key == "--warmup") {
      options.warmup = number(value, MAX_WARMUP);
    } else if (key == "--rows") {
      const auto rows = number(value, MAX_ROWS);

      check(rows > 0, "rows must be positive");
      options.sizes = {rows};
    } else if (key == "--output") {
      check(!value.empty(), "output path must not be empty");
      options.output = value;
    } else {
      throw std::runtime_error(std::format("unknown option {}", key));
    }
  }

  if (options.checkOnly) {
    options.sizes = {2, 8};
    options.samples = 3;
    options.warmup = 1;
  }

  return options;
}

struct RowNodes {
  Node<Row> row;
  Node<Label> label;
  Node<Button> button;
};

// Application startup/destruction are common runtime costs outside the timed scene phases.
struct Workload {
  Api api;
  std::unique_ptr<Application> app{take(Application::create())};
  std::unique_ptr<ManagedScene> scene;
  std::unique_ptr<StyleEngine> styles;
  Node<Widget> root;
  std::vector<RowNodes> rows;
  std::vector<ActionBinding> bindings;
  std::size_t activations{};

  explicit Workload(Api selected) : api(selected) {
  }

  void construct(std::size_t count) {
    rows.reserve(count);

    if (api == Api::LOW_LEVEL) {
      root = take(app->tree().emplaceChecked<Widget>(app->root()));
      styles = std::make_unique<StyleEngine>(app->tree());
      bindings.reserve(count);
    } else {
      scene = take(ManagedScene::create(*app, {.maxNodes = count * NODES_PER_ROW + 1}));
      root = scene->root();
    }

    for (std::size_t i = 0; i < count; ++i) {
      if (api == Api::DESCRIPTIONS) {
        ui::NodeRef<Label> label;
        ui::NodeRef<Button> button;
        auto description = ui::row(ui::label(LABEL_TEXT).reference(label),
                                   ui::button(BUTTON_TEXT, [this]() -> std::expected<void, TreeError> {
                                     ++activations;

                                     return {};
                                   }).reference(button));
        const auto row = take(ui::mount(*scene, std::move(description)));

        rows.push_back({row, label.node(), button.node()});
      } else if (api == Api::MANAGED) {
        const auto row = take(scene->emplace<Row>(root));
        const auto label = take(scene->emplace<Label>(row, take(GraphemeText::from(LABEL_TEXT))));
        const auto button = take(scene->emplace<Button>(row, take(GraphemeText::from(BUTTON_TEXT))));

        take(scene->onClick(button, [this]() -> std::expected<void, TreeError> {
          ++activations;

          return {};
        }));
        rows.push_back({row, label, button});
      } else {
        const auto row = take(app->tree().emplaceChecked<Row>(root));
        const auto label = take(app->tree().emplaceChecked<Label>(row, take(GraphemeText::from(LABEL_TEXT))));
        const auto button = take(app->tree().emplaceChecked<Button>(row, take(GraphemeText::from(BUTTON_TEXT))));

        bindings.push_back(take(bindButton(app->router(), button, [this] {
          ++activations;
        })));
        rows.push_back({row, label, button});
      }
    }
  }

  void prepare() {
    if (scene) {
      take(scene->prepare(*app, VIEWPORT));

      return;
    }

    const auto focused = take(app->router().focused());

    for (const auto &row : rows) {
      take(syncControl(app->tree(), row.label, focused));
      take(syncControl(app->tree(), row.button, focused));
    }
  }

  void theme(StyleSheet sheet) {
    if (scene) {
      take(scene->theme(std::move(sheet)));
    } else {
      take(styles->replace(std::move(sheet)));
    }
  }

  void registerStyles() {
    if (styles) {
      std::vector<StyleRegistration> registrations;

      registrations.reserve(rows.size() * 2);

      for (const auto &row : rows) {
        registrations.push_back({row.label, {.type = "Control"}});
        registrations.push_back({row.button, {.type = "Control"}});
      }

      take(styles->bindBatch(registrations));
    }

    // Managed theme flushes the same batch before replacement. Both start with an empty sheet.
    theme(modernTheme());
  }

  void verify(const StyleSheet &sheet, Node<Widget> focused) {
    const auto children = take(app->tree().inspect(root)).children;

    check(children.size() == rows.size(), "root row count differs");

    for (std::size_t i = 0; i < rows.size(); ++i) {
      const auto &row = rows[i];
      const auto descendants = take(app->tree().inspect(row.row)).children;

      check(children[i] == row.row && descendants == std::vector<Node<Widget>>{row.label, row.button},
            "row topology or ordering differs");

      for (const auto node : {Node<Control>(row.label), Node<Control>(row.button)}) {
        check(take(app->tree().inspect(node)).children.empty(), "unexpected descendant");
        take(app->tree().read(node, [&](const Control &control) {
          const auto expected = resolveStyle(sheet, {.type = "Control"}, control.styleDefaults());

          check(control.computedStyle() && *control.computedStyle() == expected, "computed style differs");
          check(control.focused() == (node == focused) && control.enabled(), "presentation differs");
          check(control.text().bytes() == (node == row.label ? LABEL_TEXT : BUTTON_TEXT), "text differs");
        }));
      }
    }
  }

  void removeRows() {
    for (const auto &row : rows) {
      if (scene) {
        take(scene->remove(row.row));
      } else {
        take(app->tree().remove(row.row));
      }
    }

    bindings.clear();
    take(app->router().focused());
  }

  void verifyRemoved() {
    check(take(app->tree().inspect(root)).children.empty(), "rows survived removal");

    for (const auto &row : rows) {
      const auto invoked = app->router().invoke(row.button, {Action::ACTIVATE});

      check(!invoked && invoked.error() == TreeError::STALE_NODE, "removed callback remains callable");
      check(!app->tree().inspect(row.label) && !app->tree().inspect(row.row), "removed node remains live");
    }

    check(activations == rows.size(), "callback count differs after removal");
  }

  void cleanup() {
    if (scene) {
      take(scene->close());
      scene.reset();
    } else {
      take(app->tree().remove(root));
      styles.reset();
    }

    // Collect router-retained disconnected callbacks before the phase ends.
    take(app->router().focused());
    std::vector<RowNodes>().swap(rows);
    std::vector<ActionBinding>().swap(bindings);
  }
};

template <class Function> std::int64_t measure(Function &&operation) {
  const auto begin = Clock::now();

  operation();

  return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count();
}

std::array<std::int64_t, PHASE_COUNT> run(Api api, std::size_t count) {
  Workload workload(api);
  std::array<std::int64_t, PHASE_COUNT> result{};

  result[std::to_underlying(Phase::CONSTRUCTION)] = measure([&] {
    workload.construct(count);
  });
  result[std::to_underlying(Phase::INITIAL_STYLE_REGISTER)] = measure([&] {
    workload.registerStyles();
  });
  result[std::to_underlying(Phase::INITIAL_PREPARE)] = measure([&] {
    workload.prepare();
  });
  workload.verify(modernTheme(), workload.rows.front().button);

  for (const auto &row : workload.rows) {
    take(workload.app->router().invoke(row.button, {Action::ACTIVATE}));
  }

  check(workload.activations == count, "button activation count differs");
  take(workload.app->router().setFocus(workload.rows.back().button));
  take(workload.app->tree().invalidate(workload.root, Invalidation::PAINT));
  result[std::to_underlying(Phase::DIRTY_PREPARE)] = measure([&] {
    workload.prepare();
  });
  workload.verify(modernTheme(), workload.rows.back().button);
  result[std::to_underlying(Phase::THEME_SWITCH_PREPARE)] = measure([&] {
    workload.theme(turboTheme());
    workload.prepare();
  });
  workload.verify(turboTheme(), workload.rows.back().button);
  result[std::to_underlying(Phase::REMOVE_ROWS)] = measure([&] {
    workload.removeRows();
  });
  workload.verifyRemoved();
  result[std::to_underlying(Phase::CLEANUP)] = measure([&] {
    workload.cleanup();
  });
  check(!workload.app->tree().inspect(workload.root), "scene root survived cleanup");
  check(take(workload.app->tree().inspect(workload.app->root())).children.empty(), "scene leaked nodes");

  return result;
}

struct Samples {
  std::array<std::vector<std::int64_t>, PHASE_COUNT> phases;
};

std::string summarize(const std::vector<std::int64_t> &values) {
  auto sorted = values;

  std::ranges::sort(sorted);
  check(!sorted.empty() && sorted.front() >= 0, "invalid clock sample");

  const auto p50 = sorted[(sorted.size() * 50 + 99) / 100 - 1];
  const auto p95 = sorted[(sorted.size() * 95 + 99) / 100 - 1];
  auto result = std::format("{{\"count\":{},\"p50\":{},\"p95\":{},\"min\":{},\"max\":{},\"samples\":[", values.size(),
                            p50, p95, sorted.front(), sorted.back());

  for (std::size_t i = 0; i < values.size(); ++i) {
    result += std::format("{}{}", i ? "," : "", values[i]);
  }

  return result + "]}";
}

std::string compiler() {
#if defined(__clang__)
  return std::format("Clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(_MSC_VER)
  return std::format("MSVC {}", _MSC_FULL_VER);
#elif defined(__GNUC__)
  return std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
  return "unknown";
#endif
}

std::string execute(const Options &options) {
#ifdef PORTALS_BENCHMARK_BUILD_TYPE
  constexpr std::string_view BUILD = PORTALS_BENCHMARK_BUILD_TYPE;
#else
  constexpr std::string_view BUILD = "unspecified";
#endif
#ifdef NDEBUG
  constexpr bool ASSERTIONS = false;
#else
  constexpr bool ASSERTIONS = true;
#endif

  auto output =
    std::format("{{\"schema\":1,\"complete\":true,\"correctness_verified\":true,\"check_only\":{},"
                "\"compiler\":\"{}\",\"build\":\"{}\",\"assertions_enabled\":{},\"pointer_bits\":{},"
                "\"clock\":\"steady_clock\",\"unit\":\"ns\",\"percentile\":\"nearest_rank\","
                "\"samples_per_case\":{},\"warmup_per_case\":{},\"cases\":[",
                options.checkOnly, compiler(), BUILD, ASSERTIONS, sizeof(void *) * 8, options.samples, options.warmup);
  bool firstCase = true;

  for (const auto count : options.sizes) {
    std::array<Samples, API_COUNT> collected;

    for (auto &samples : collected) {
      for (auto &phase : samples.phases) {
        phase.reserve(options.samples);
      }
    }

    // Rotate API order each round so a single API is not always first after warmup.
    for (std::size_t round = 0; round < options.warmup + options.samples; ++round) {
      for (std::size_t offset = 0; offset < API_COUNT; ++offset) {
        const auto index = (round + offset) % API_COUNT;
        const auto sample = run(APIS[index].api, count);

        if (round >= options.warmup) {
          for (std::size_t phase = 0; phase < PHASE_COUNT; ++phase) {
            collected[index].phases[phase].push_back(sample[phase]);
          }
        }
      }
    }

    for (std::size_t index = 0; index < API_COUNT; ++index) {
      output += std::format("{}{{\"api\":\"{}\",\"rows\":{},\"scene_nodes\":{},\"styled_controls\":{},\"phases\":{{",
                            firstCase ? "" : ",", APIS[index].name, count, count * NODES_PER_ROW + 1, count * 2);
      firstCase = false;

      for (std::size_t phase = 0; phase < PHASE_COUNT; ++phase) {
        output +=
          std::format("{}\"{}\":{}", phase ? "," : "", PHASE_NAMES[phase], summarize(collected[index].phases[phase]));
      }

      output += "}}";
    }
  }

  return output + "]}\n";
}
} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse(argc, argv);
    const auto output = execute(options);

    if (options.output.empty()) {
      std::cout << output;
      check(static_cast<bool>(std::cout), "cannot write stdout");
    } else {
      std::ofstream file(options.output, std::ios::binary);

      file << output;
      file.close();
      check(static_cast<bool>(file), "cannot write output file");
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }

  return 0;
}
