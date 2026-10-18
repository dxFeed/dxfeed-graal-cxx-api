// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../../examples/replay/terminal.hpp"
#include "../../tests/support/vt_oracle.hpp"
#include "measurement.hpp"
#include <charconv>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#else
#  include <time.h>
#  include <unistd.h>
#endif

namespace {
using namespace benchmark;
using example::take;
using Clock = std::chrono::steady_clock;
constexpr std::size_t PHASE_COUNT = 11;
constexpr std::size_t CAPTURE_LIMIT = 16;
constexpr std::size_t ENCODING_REPETITIONS = 20;

struct Options {
  dashboard::Options workload;
  std::uint32_t fps{30};
  std::uint64_t warmupMs{5000}, durationMs{30000};
  std::size_t capacity{65536};
  std::string transport{"headless"}, output;
  bool diagnostics{}, ansi16{}, encodingOnly{};
};

std::uint64_t number(std::string_view text) {
  std::uint64_t value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);

  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::runtime_error("invalid unsigned integer");
  }

  return value;
}

Options parse(int argc, char **argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string_view key(argv[i]);

    if (key == "--ascii") {
      options.workload.ascii = true;
      continue;
    }

    if (key == "--ansi16") {
      options.ansi16 = true;
      continue;
    }

    if (key == "--encoding-only") {
      options.encodingOnly = true;
      continue;
    }

    if (i + 1 == argc) {
      throw std::runtime_error("missing option value");
    }

    const std::string_view value(argv[++i]);

    if (key == "--output") {
      options.output = value;
    } else if (key == "--transport") {
      options.transport = value;
    } else if (key == "--diagnostics") {
      if (value != "enabled" && value != "disabled") {
        throw std::runtime_error("diagnostics must be enabled or disabled");
      }

      options.diagnostics = value == "enabled";
    } else if (key == "--rate") {
      const auto rate = number(value);

      if (rate > 1000000) {
        throw std::runtime_error("rate exceeds dashboard limit");
      }

      options.workload.rate = static_cast<std::uint32_t>(rate);
    } else if (key == "--seed") {
      options.workload.seed = number(value);
    } else if (key == "--rows") {
      options.workload.rows = number(value);
    } else if (key == "--fps") {
      const auto fps = number(value);

      if (!fps || fps > 1000) {
        throw std::runtime_error("fps must be 1..1000");
      }

      options.fps = static_cast<std::uint32_t>(fps);
    } else if (key == "--warmup-ms") {
      options.warmupMs = number(value);
    } else if (key == "--duration-ms") {
      options.durationMs = number(value);
    } else if (key == "--delay-ms") {
      options.workload.delayMs = number(value);
    } else if (key == "--sample-capacity") {
      const auto capacity = number(value);

      if (!capacity || capacity > 1000000) {
        throw std::runtime_error("sample capacity must be 1..1000000");
      }

      options.capacity = static_cast<std::size_t>(capacity);
    } else {
      throw std::runtime_error("unknown option " + std::string(key));
    }
  }

  if (options.output.empty() || !options.durationMs || options.warmupMs > 3600000 || options.durationMs > 3600000 ||
      options.workload.rows == 0 || options.workload.rows > 1000000000 ||
      (options.transport != "headless" && options.transport != "native")) {
    throw std::runtime_error("invalid workload or missing --output");
  }

  return options;
}

std::optional<std::uint64_t> processCpuNs(int &error) {
#ifdef _WIN32
  FILETIME creation{}, exit{}, kernel{}, user{};

  if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
    error = static_cast<int>(GetLastError());

    return {};
  }

  const auto ticks = [](FILETIME value) {
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
  };

  return (ticks(kernel) + ticks(user)) * 100;
#else
  timespec value{};

  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value)) {
    error = errno;

    return {};
  }

  return static_cast<std::uint64_t>(value.tv_sec) * 1000000000 + static_cast<std::uint64_t>(value.tv_nsec);
#endif
}

struct Boundary {
  Clock::time_point wall{};
  std::optional<std::uint64_t> cpu;
  SchedulerMetrics scheduler;
  OutputCounters output;
  std::uint64_t generated{}, scheduled{}, frames{};
  bool reached{};
  int cpuError{};
};

struct Measurements {
  explicit Measurements(std::size_t capacity) : ingress(capacity), reads(capacity) {
    phases.reserve(PHASE_COUNT);

    for (std::size_t i = 0; i < PHASE_COUNT; ++i) {
      phases.emplace_back(capacity);
    }
  }

  static void observe(void *context, ApplicationPhase phase, std::chrono::nanoseconds elapsed) {
    auto &self = *static_cast<Measurements *>(context);

    if (phase == ApplicationPhase::SUBMIT) {
      ++self.frames;
    }

    self.phases[static_cast<std::size_t>(phase)].add(static_cast<std::uint64_t>(elapsed.count()),
                                                     self.active || phase == ApplicationPhase::STOP ||
                                                       phase == ApplicationPhase::CLOSE);
  }

  static void displayIngress(void *context, SchedulerTime elapsed) {
    auto &self = *static_cast<Measurements *>(context);

    self.ingress.add(elapsed, self.active);
  }

  Boundary snapshot(Application &app, dashboard::Scene &scene, const CountedTerminal &terminal) const {
    Boundary result;

    result.wall = Clock::now();
    result.cpu = processCpuNs(result.cpuError);
    result.scheduler = app.scheduler().metrics();
    result.output = terminal.counters;
    result.generated = scene.tickCount();
    result.scheduled = scene.scheduledTicks();
    result.frames = frames;
    result.reached = true;

    return result;
  }

  std::vector<Samples> phases;
  Samples ingress, reads;
  Boundary begin, end;
  std::uint64_t frames{};
  bool active{};
};

class ObservedScene final : public ApplicationScene {
public:
  explicit ObservedScene(dashboard::Scene &scene) : scene_(scene) {
  }

  std::expected<void, TreeError> prepare(Application &app, CellSize size) override {
    viewport = size;

    return scene_.prepare(app, size);
  }

  bool usesPreparedLayout() const noexcept override {
    return scene_.usesPreparedLayout();
  }

  std::expected<void, TreeError> prepareViews(Application &app, const LayoutSnapshot &snapshot) override {
    return scene_.prepareViews(app, snapshot);
  }

  std::expected<bool, TreeError> input(Application &app, const InputEvent &event) override {
    quit = quit || example::quitKey(event);

    return scene_.input(app, event);
  }

  void stop(Application &app) override {
    // Managed cleanup retires the table. Preserve its final observation before stop.
    auto captured = app.tree().read(scene_.table(), [&](const auto &table) {
      finalFirstRow = table.firstRow();
      finalRangeState = table.rangeState();
      finalSelection = table.selection();
    });

    const auto begin = Clock::now();

    scene_.stopProvider();
    providerStopNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count();
    providerStopped = scene_.stoppedAt().has_value();
    scene_.stop(app);
    take(captured);
  }

  CellSize viewport{120, 40};
  std::uint64_t finalFirstRow{};
  RangeState finalRangeState{RangeState::PENDING};
  std::optional<RowId> finalSelection;
  std::int64_t providerStopNs{};
  bool providerStopped{}, quit{};

private:
  dashboard::Scene &scene_;
};

void quoted(std::ostream &out, std::string_view value) {
  out << '"';

  for (const auto byte : value) {
    const auto c = static_cast<unsigned char>(byte);

    if (c == '"' || c == '\\') {
      out << '\\' << byte;
    } else if (c < 32) {
      out << "\\u00" << "0123456789abcdef"[c >> 4] << "0123456789abcdef"[c & 15];
    } else {
      out << byte;
    }
  }

  out << '"';
}

void nullable(std::ostream &out, std::optional<std::uint64_t> value) {
  if (value) {
    out << *value;
  } else {
    out << "null";
  }
}

void samples(std::ostream &out, const Samples &data, std::string_view units) {
  const auto stats = data.summary();

  out << "{\"unit\":";
  quoted(out, units);
  out << ",\"count\":" << data.values().size() << ",\"overflow\":" << data.overflow()
      << ",\"percentiles_complete\":" << (!data.values().empty() && data.overflow() == 0) << ",\"p50\":";
  nullable(out, stats.p50);
  out << ",\"p95\":";
  nullable(out, stats.p95);
  out << ",\"max\":";
  nullable(out, stats.maximum);
  out << ",\"raw\":[";

  for (std::size_t i = 0; i < data.values().size(); ++i) {
    out << (i ? "," : "") << data.values()[i];
  }

  out << "]}";
}

void queue(std::ostream &out, const SchedulerQueueMetrics &q) {
  out << "{\"pending\":" << q.pending << ",\"high_water_lifetime\":" << q.high_water << ",\"accepted\":" << q.accepted
      << ",\"rejected\":" << q.rejected << ",\"replaced\":" << q.replaced << ",\"delivered\":" << q.delivered
      << ",\"cancelled\":" << q.cancelled << ",\"last_latency_ms\":" << q.last_latency
      << ",\"max_latency_lifetime_ms\":" << q.max_latency << ",\"total_latency_ms\":" << q.total_latency << '}';
}

void output(std::ostream &out, const OutputCounters &o) {
  out << "{\"nonempty_submitted_frames\":" << o.submitted << ",\"encoded_bytes\":" << o.encoded
      << ",\"flushes\":" << o.flushes << ",\"confirmed_bytes\":" << o.confirmed
      << ",\"completed_transport_frames\":" << o.completedFrames << ",\"partial_flushes\":" << o.partial
      << ",\"uncertain_flushes\":" << o.uncertain << ",\"pending_bytes\":" << o.pending << ",\"valid\":" << o.valid
      << '}';
}

void boundary(std::ostream &out, const Boundary &b) {
  out << "{\"reached\":" << b.reached
      << ",\"steady_ns\":" << std::chrono::duration_cast<std::chrono::nanoseconds>(b.wall.time_since_epoch()).count()
      << ",\"process_cpu_ns\":";
  nullable(out, b.cpu);
  out << ",\"cpu_read_error\":" << b.cpuError << ",\"generated\":" << b.generated << ",\"scheduled\":" << b.scheduled
      << ",\"backlog\":" << b.scheduled - b.generated << ",\"accepted_frames\":" << b.frames << ",\"output\":";
  output(out, b.output);
  out << ",\"scheduler_data\":";
  queue(out, b.scheduler.data);
  out << ",\"scheduler_control\":";
  queue(out, b.scheduler.control);
  out << ",\"scheduler_cycles\":" << b.scheduler.cycles << ",\"timer_callbacks\":" << b.scheduler.timer_callbacks
      << ",\"callback_failures\":" << b.scheduler.callback_failures << '}';
}

int writeResult(std::ostream &out, const Options &options, const Measurements &m, Application &app,
                dashboard::Scene &scene, const ObservedScene &observed, const CountedTerminal &terminal,
                CellSize initialSize, std::uint64_t nativeOpenNs,
                const std::expected<ApplicationStats, ApplicationError> &result,
                const DiagnosticSnapshot &diagnostics) {
  const bool window = m.begin.reached && m.end.reached;
  const bool cpu = window && m.begin.cpu && m.end.cpu && *m.end.cpu >= *m.begin.cpu;
  const auto wallNs =
    window ? std::chrono::duration_cast<std::chrono::nanoseconds>(m.end.wall - m.begin.wall).count() : 0;
  const auto cpuNs = cpu ? *m.end.cpu - *m.begin.cpu : 0;
  const bool fullDuration = window && wallNs >= static_cast<std::int64_t>(options.durationMs) * 1000000;
  const bool lossless = verifyOhlc(scene.aggregate(), scene.tickCount(), options.workload.seed);
  const bool accounting = result && result->bytes == terminal.counters.confirmed && result->frames == m.frames;
  const bool complete = result && fullDuration && cpu && terminal.closed && terminal.counters.complete() && lossless &&
                        scene.coherent() && accounting;
  const bool retained =
    !m.ingress.overflow() && !m.reads.overflow() && std::ranges::all_of(m.phases, [](const auto &series) {
      return series.overflow() == 0;
    });
  const bool frameCap =
    !window || m.end.frames - m.begin.frames <=
                 (static_cast<std::uint64_t>(std::max<std::int64_t>(0, wallNs)) * options.fps) / 1000000000 + 1;
  const bool fixedViewport = initialSize == CellSize{120, 40} && terminal.finalSize == initialSize &&
                             observed.viewport == initialSize && !terminal.resizes;

  out << std::boolalpha << std::setprecision(12) << "{\"schema\":1,\"kind\":\"application\",\"complete\":" << complete
      << ",\"application_ok\":" << result.has_value() << ",\"exit_code\":"
      << (complete ? 0
          : result ? 2
                   : 1)
      << ",\"exit_reason\":";
  quoted(out, !result         ? "application_error"
              : m.end.reached ? "observation_deadline"
              : observed.quit ? "quit_input"
              : terminal.eof  ? "eof"
                              : "early_stop");
  out << ",\"error_phase\":";

  if (!result) {
    quoted(out, example::phaseName(result.error().phase));
  } else {
    out << "null";
  }

#ifdef _WIN32
  out << ",\"platform\":\"Windows\",\"process_cpu_reader\":\"GetProcessTimes kernel+user\"";
#else
  out << ",\"platform\":\"Linux\",\"process_cpu_reader\":\"CLOCK_PROCESS_CPUTIME_ID\"";
#endif
#ifdef _MSC_VER
  out << ",\"compiler\":\"MSVC " << _MSC_FULL_VER << "\"";
#else
  out << ",\"compiler\":";
  quoted(out, __VERSION__);
#endif
#ifdef NDEBUG
  out << ",\"build\":\"Release\"";
#else
  out << ",\"build\":\"Debug\"";
#endif
  out << ",\"transport\":";
  quoted(out, options.transport);
  out << ",\"generator\":\"dashboard::syntheticPrice index+seed v1\",\"configured_rate\":" << options.workload.rate
      << ",\"seed\":" << options.workload.seed << ",\"rows\":" << options.workload.rows
      << ",\"fps_cap\":" << options.fps << ",\"warmup_ms\":" << options.warmupMs
      << ",\"duration_ms\":" << options.durationMs << ",\"delay_ms\":" << options.workload.delayMs
      << ",\"ascii\":" << options.workload.ascii << ",\"ansi16\":" << options.ansi16
      << ",\"diagnostics_enabled\":" << options.diagnostics << ",\"sample_capacity\":" << options.capacity
      << ",\"sample_boundary\":\"phase completion within observation; shutdown separately\""
      << ",\"all_samples_retained\":" << retained << ",\"frame_cap_respected\":" << frameCap
      << ",\"fixed_viewport_baseline\":" << fixedViewport
      << ",\"warmup_had_completed_frame\":" << (m.begin.reached && m.begin.output.completedFrames > 0)
      << ",\"native_open_ns\":" << nativeOpenNs << ",\"window\":{\"wall_ns\":" << wallNs << ",\"process_cpu_ns\":";
  nullable(out, cpu ? std::optional(cpuNs) : std::nullopt);
  out << ",\"cpu_percent_one_cpu\":";

  if (cpu && wallNs > 0) {
    out << static_cast<double>(cpuNs) * 100 / static_cast<double>(wallNs);
  } else {
    out << "null";
  }

  out << ",\"generated_ticks_per_second\":";

  if (window && wallNs > 0) {
    out << static_cast<double>(m.end.generated - m.begin.generated) * 1e9 / static_cast<double>(wallNs);
  } else {
    out << "null";
  }

  out << ",\"confirmed_bytes\":" << (window ? m.end.output.confirmed - m.begin.output.confirmed : 0)
      << ",\"encoded_bytes\":" << (window ? m.end.output.encoded - m.begin.output.encoded : 0)
      << ",\"accepted_frames\":" << (window ? m.end.frames - m.begin.frames : 0) << ",\"confirmed_bytes_per_second\":";

  if (window && wallNs > 0) {
    out << static_cast<double>(m.end.output.confirmed - m.begin.output.confirmed) * 1e9 / static_cast<double>(wallNs);
  } else {
    out << "null";
  }

  out << ",\"begin\":";
  boundary(out, m.begin);
  out << ",\"end\":";
  boundary(out, m.end);
  out << "},\"phase_samples_ns\":{";

  for (std::size_t i = 0; i < PHASE_COUNT; ++i) {
    out << (i ? "," : "");
    quoted(out, example::phaseName(static_cast<ApplicationPhase>(i)));
    out << ':';
    samples(out, m.phases[i], "ns");
  }

  out << "},\"transport_read_ns\":";
  samples(out, m.reads, "ns including wait");
  out << ",\"transport_read_calls_lifetime\":" << terminal.readCalls;
  out << ",\"dashboard_display_ingress_ms\":";
  samples(out, m.ingress, "ms");
  out << ",\"runner_input_control_ns\":";
  samples(out, m.phases[static_cast<std::size_t>(ApplicationPhase::CONTROL_LATENCY)], "ns (logical ms * 1000000)");
  out << ",\"error_cause\":";

  if (!result) {
    std::visit(
      [&](const auto &cause) {
        using T = std::decay_t<decltype(cause)>;

        if constexpr (std::same_as<T, TerminalError>) {
          out << "{\"kind\":" << static_cast<int>(cause.kind) << ",\"native_code\":" << cause.native_code
              << ",\"restoration_code\":" << cause.restoration_code << '}';
        } else if constexpr (std::same_as<T, RenderError>) {
          out << "{\"kind\":" << static_cast<int>(cause.kind)
              << ",\"buffer_error\":" << static_cast<int>(cause.buffer_error) << '}';
        } else {
          out << static_cast<int>(cause);
        }
      },
      result.error().cause);
  } else {
    out << "null";
  }

  out << ",\"scheduler_final_data_lifetime\":";
  queue(out, app.scheduler().metrics().data);
  out << ",\"scheduler_final_control_lifetime\":";
  queue(out, app.scheduler().metrics().control);
  out << ",\"runner_accounting_matches\":" << accounting << ",\"output_final_lifetime\":";
  output(out, terminal.counters);
  out << ",\"shutdown\":{\"provider_synchronous_timer_stop_ns\":" << observed.providerStopNs
      << ",\"provider_stopped\":" << observed.providerStopped << ",\"terminal_closed\":" << terminal.closed
      << ",\"close_label\":\"join+restore combined; see CLOSE phase\",\"pure_os_restore_ns\":null}"
      << ",\"diagnostics\":{\"capacity\":" << diagnostics.options.capacity
      << ",\"retained\":" << diagnostics.records.size() << ",\"dropped_overflow\":" << diagnostics.droppedOverflow
      << ",\"dropped_contention\":" << diagnostics.droppedContention
      << ",\"counters_saturated\":" << diagnostics.countersSaturated << ",\"incomplete\":" << diagnostics.incomplete
      << '}' << ",\"acceptance\":{\"coherent\":" << scene.coherent() << ",\"lossless_ohlc\":" << lossless
      << ",\"displayed_revision\":" << scene.displayedRevision() << ",\"generated\":" << scene.tickCount()
      << ",\"scheduled\":" << scene.scheduledTicks() << ",\"backlog\":" << scene.pendingTicks()
      << ",\"max_prepared_rows\":" << scene.maxPreparedRows() << ",\"pending_visible\":" << scene.pendingVisible()
      << ",\"theme\":\"" << (scene.turbo() ? "Turbo" : "Modern") << "\",\"note\":";
  quoted(out, take(scene.note().snapshot())->value);
  out << ",\"viewport\":{\"width\":" << terminal.finalSize.width << ",\"height\":" << terminal.finalSize.height
      << "},\"initial_viewport\":{\"width\":" << initialSize.width << ",\"height\":" << initialSize.height
      << "},\"resize_batches\":" << terminal.resizes;
  out << ",\"first_row\":" << observed.finalFirstRow
      << ",\"range_state\":" << static_cast<int>(observed.finalRangeState) << ",\"selection\":";

  if (const auto row = observed.finalSelection) {
    out << "{\"value\":" << row->value << ",\"generation\":" << row->generation << '}';
  } else {
    out << "null";
  }

  const auto aggregate = scene.aggregate();

  out << ",\"ohlc\":{\"open\":" << aggregate.open << ",\"high\":" << aggregate.high << ",\"low\":" << aggregate.low
      << ",\"close\":" << aggregate.close << "}}}\n";

  return complete ? 0 : result ? 2 : 1;
}

class CaptureReplay final : public example::ReplayTerminal {
public:
  CaptureReplay() : ReplayTerminal({}, 400, {120, 40}) {
    batches.reserve(CAPTURE_LIMIT);
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    if (batches.size() == CAPTURE_LIMIT) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_LIMIT});
    }

    batches.push_back(bytes);

    return ReplayTerminal::submit(std::move(bytes));
  }

  std::vector<std::string> batches;
};

void encodingBenchmark(std::ostream &out, const Options &options) {
  CaptureReplay capture;
  ApplicationOptions config;
  config.writer.colorMode = options.ansi16 ? ColorMode::ANSI16 : ColorMode::TRUE_COLOR;
  auto app = take(Application::create(config, capture.clock()));
  dashboard::Options workload;
  workload.seed = options.workload.seed;
  workload.ascii = true;
  dashboard::Scene scene(*app, workload);

  take(app->run(scene, capture));

  // Capture only complete encoded transactions in a separate finite replay run.
  // The independent VT machine materializes owning scene text before any timing.
  test_support::VtOracle original(120, 40), fullOracle(120, 40), diffOracle(120, 40);
  std::vector<CellBuffer> frames;
  std::vector<std::vector<GraphemeText>> text;
  std::vector<std::vector<RenderCommand>> commands;
  frames.reserve(CAPTURE_LIMIT);
  text.reserve(CAPTURE_LIMIT);
  commands.reserve(CAPTURE_LIMIT);

  for (const auto &batch : capture.batches) {
    original.feed(batch);
    const auto snapshot = original.snapshot();
    auto frame = take(CellBuffer::from({120, 40}));
    auto &owned = text.emplace_back();
    auto &saved = commands.emplace_back();

    owned.reserve(snapshot.cells.size());
    saved.reserve(snapshot.cells.size());

    for (std::size_t i = 0; i < snapshot.cells.size(); ++i) {
      const auto &pixel = snapshot.cells[i];

      if (pixel.tail) {
        continue;
      }

      const CellPoint point{static_cast<std::int32_t>(i % 120), static_cast<std::int32_t>(i / 120)};
      auto &glyph = owned.emplace_back(take(GraphemeText::from(pixel.glyph)));

      take(frame.put(point, glyph, 0, pixel.style));
      saved.emplace_back(TextCommand{point, glyph, take(CellRect::from({0, 0}, {120, 40})), pixel.style, 0});
    }

    const auto full = take(encodeDiff(nullptr, frame, 1024 * 1024, config.writer.colorMode));
    const auto diff =
      take(encodeDiff(frames.empty() ? nullptr : &frames.back(), frame, 1024 * 1024, config.writer.colorMode));

    fullOracle.feed(full);
    diffOracle.feed(diff);

    if (fullOracle.snapshot() != snapshot || diffOracle.snapshot() != snapshot ||
        !take(encodeDiff(&frame, frame)).empty()) {
      throw std::runtime_error("independent full/diff VT oracle mismatch");
    }

    frames.push_back(std::move(frame));
  }

  if (frames.size() < 2) {
    throw std::runtime_error("insufficient captured frames");
  }

  Samples fullTimes(CAPTURE_LIMIT * ENCODING_REPETITIONS), diffTimes(CAPTURE_LIMIT * ENCODING_REPETITIONS);
  Samples renderTimes(CAPTURE_LIMIT * ENCODING_REPETITIONS);
  std::uint64_t fullBytes{}, diffBytes{}, checksum{};

  for (std::size_t repeat = 0; repeat < ENCODING_REPETITIONS; ++repeat) {
    for (std::size_t i = 0; i < frames.size(); ++i) {
      auto begin = Clock::now();
      auto full = take(encodeDiff(nullptr, frames[i], 1024 * 1024, config.writer.colorMode));

      fullTimes.add(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count(), true);
      begin = Clock::now();
      auto diff = take(encodeDiff(i ? &frames[i - 1] : nullptr, frames[i], 1024 * 1024, config.writer.colorMode));

      diffTimes.add(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count(), true);
      begin = Clock::now();
      auto rendered = take(renderFullFrame({120, 40}, commands[i]));

      renderTimes.add(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count(), true);
      fullBytes += full.size();
      diffBytes += diff.size();
      checksum += full.size() + diff.size() + rendered.memory().glyph_bytes;
    }
  }

  out << std::boolalpha << "{\"schema\":1,\"kind\":\"separate_encoding\",\"complete\":true,\"oracle_verified\":true"
      << ",\"capture\":\"owning VT-oracle reconstruction of finite Application/dashboard replay\",\"frames\":"
      << frames.size() << ",\"repetitions\":" << ENCODING_REPETITIONS << ",\"ascii\":true,\"ansi16\":" << options.ansi16
      << ",\"full_bytes\":" << fullBytes << ",\"diff_bytes\":" << diffBytes << ",\"checksum\":" << checksum
      << ",\"encode_full_ns\":";
  samples(out, fullTimes, "ns");
  out << ",\"encode_diff_ns\":";
  samples(out, diffTimes, "ns");
  out << ",\"render_saved_cell_scene_ns\":";
  samples(out, renderTimes, "ns");
  out << "}\n";
}

int run(const Options &options, std::ostream &report) {
  if (options.encodingOnly) {
    encodingBenchmark(report, options);

    return 0;
  }

  Measurements measurements(options.capacity);
  auto diagnostics = std::make_shared<Diagnostics>();

  if (options.diagnostics) {
    take(diagnostics->enable({65536}));
  }

  ApplicationOptions config;
  config.framesPerSecond = options.fps;
  config.writer.colorMode = options.ansi16 ? ColorMode::ANSI16 : ColorMode::TRUE_COLOR;
  config.scheduler.diagnostics = diagnostics;
  config.observer = {Measurements::observe, &measurements};
  auto app = take(Application::create(config));
  auto workload = options.workload;

  workload.ingressObserver = Measurements::displayIngress;
  workload.ingressContext = &measurements;

  dashboard::Scene scene(*app, workload);
  ObservedScene observed(scene);
  std::unique_ptr<ApplicationTerminal> transport;
  std::uint64_t nativeOpenNs{};

  if (options.transport == "native") {
#ifdef _WIN32
    const NativeTerminalHandles handles{reinterpret_cast<std::intptr_t>(GetStdHandle(STD_INPUT_HANDLE)),
                                        reinterpret_cast<std::intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE))};
#else
    const NativeTerminalHandles handles{STDIN_FILENO, STDOUT_FILENO};
#endif
    const auto begin = Clock::now();
    auto session =
      openNativeApplicationTerminal(handles, config.terminal, config.outputSlice, config.nativeMouseReporting);

    nativeOpenNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count();

    if (!session) {
      report << "{\"schema\":1,\"complete\":false,\"application_ok\":false,\"exit_code\":1,"
                "\"exit_reason\":\"native_open_failed\",\"terminal_error_kind\":"
             << static_cast<int>(session.error().kind) << ",\"native_code\":" << session.error().native_code << "}\n";

      return 1;
    }

    transport = std::move(*session);
  } else {
    transport = std::make_unique<HeadlessTerminal>();
  }

  CountedTerminal terminal(*transport, &measurements.reads, &measurements.active);
  const auto initialSize = take(terminal.size());

  terminal.finalSize = initialSize;

  auto scope = take(app->scheduler().makeScope());
  const auto origin = app->now();
  Subscription end;
  auto start = take(app->scheduler().at(scope, origin + options.warmupMs, [&] {
    measurements.begin = measurements.snapshot(*app, scene, terminal);
    measurements.active = true;

    // Round up from the real boundary so an integer-ms scheduler deadline cannot
    // accidentally shorten the requested wall observation by a fractional ms.
    const auto stopAt = std::chrono::ceil<std::chrono::milliseconds>(measurements.begin.wall.time_since_epoch() +
                                                                     std::chrono::milliseconds(options.durationMs))
                          .count();

    end = take(app->scheduler().at(scope, static_cast<SchedulerTime>(stopAt), [&] {
      measurements.end = measurements.snapshot(*app, scene, terminal);
      measurements.active = false;
      app->requestStop();
    }));
  }));

  const auto result = app->run(observed, terminal);

  diagnostics->disable();

  return writeResult(report, options, measurements, *app, scene, observed, terminal, initialSize, nativeOpenNs, result,
                     diagnostics->snapshot());
}
} // namespace

int main(int argc, char **argv) {
  std::string outputPath;

  try {
    const auto options = parse(argc, argv);

    outputPath = options.output;
    std::ostringstream report;
    const auto result = run(options, report);
    std::ofstream file(options.output, std::ios::binary);

    file << report.str();
    file.close();

    if (!file) {
      throw std::runtime_error("result file write failed");
    }

    return result;
  } catch (const std::exception &error) {
    // All run-owned terminal objects have unwound before any report is opened.
    if (!outputPath.empty()) {
      std::ofstream file(outputPath, std::ios::binary);

      file << "{\"schema\":1,\"complete\":false,\"application_ok\":false,\"exit_code\":1,"
              "\"exit_reason\":\"setup_or_export_exception\",\"message\":";
      quoted(file, error.what());
      file << "}\n";
    }

    std::cerr << "MVP benchmark: " << error.what() << '\n';

    return 1;
  }
}
